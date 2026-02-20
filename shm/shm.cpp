// shm/shm.cpp
#include "shm.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>

namespace {
std::mutex g_log_mtx;
}  // namespace

// ------------------------- API Functions -------------------------
uint32_t getVectorSize(const Packet* p) { return p->vec_size; }

void setVectorSize(Packet* p, uint32_t size) { p->vec_size = std::min(size, MAX_VEC_SIZE); }

int32_t getData(const Packet* p, int vec_id, std::size_t index) {
  return (vec_id == 0) ? p->vec_a[index] : p->vec_b[index];
}

void setData(Packet* p, int32_t v, int vec_id, std::size_t index) {
  if (vec_id == 0)
    p->vec_a[index] = v;
  else
    p->vec_b[index] = v;
}

void getVectorData(const Packet* p, int32_t* dst, int vec_id, uint32_t size) {
  size               = std::min(size, MAX_VEC_SIZE);
  const int32_t* src = (vec_id == 0) ? p->vec_a : p->vec_b;
  std::memcpy(dst, src, size * sizeof(int32_t));
}

void setVectorData(Packet* p, const int32_t* src, int vec_id, uint32_t size) {
  size         = std::min(size, MAX_VEC_SIZE);
  int32_t* dst = (vec_id == 0) ? p->vec_a : p->vec_b;
  std::memcpy(dst, src, size * sizeof(int32_t));
}

// ------------------------- Helper Functions (Internal) -------------------------
// Bit manipulation helpers
static inline bool testBit(uint32_t v, int bit) { return (v >> bit) & 1u; }
static inline void setBit(uint32_t& v, int bit) { v |= (1u << bit); }
static inline void clrBit(uint32_t& v, int bit) { v &= ~(1u << bit); }

// ------------------------- SharedMemorySegment Implementation -------------------------
SharedMemorySegment::SharedMemorySegment(const std::string& shm_name, bool clear) : name(shm_name) {
  fd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
  if (fd < 0) throw std::runtime_error("shm_open failed: " + name);

  if (ftruncate(fd, SHM_SIZE) != 0) throw std::runtime_error("ftruncate failed: " + name);

  void* mapped = mmap(nullptr, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mapped == MAP_FAILED) throw std::runtime_error("mmap failed: " + name);

  base = static_cast<Packet*>(mapped);

  if (clear) std::memset(base, 0, SHM_SIZE);
}

SharedMemorySegment::~SharedMemorySegment() {
  if (base) munmap(base, SHM_SIZE);
  if (fd >= 0) close(fd);
}

uint32_t SharedMemorySegment::flags() const { return base->flags; }
void     SharedMemorySegment::writeFlags(uint32_t v) { base->flags = v; }

void SharedMemorySegment::setFlag(int bit) {
  auto v = flags();
  setBit(v, bit);
  writeFlags(v);
}

void SharedMemorySegment::clearFlag(int bit) {
  auto v = flags();
  clrBit(v, bit);
  writeFlags(v);
}

bool SharedMemorySegment::isFlagSet(int bit) const { return testBit(flags(), bit); }

// ------------------------- Thread Helper Functions -----------------------
void logLine(const std::string& s) {
  std::lock_guard<std::mutex> lk(g_log_mtx);
  std::cout << s << std::endl;
}