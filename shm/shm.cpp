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

// ------------------------- SharedMemorySegment Implementation -------------------------
SharedMemorySegment::SharedMemorySegment(const std::string& shm_name, bool clear, bool unlink)
    : name(shm_name), unlink_on_destroy(unlink) {
  fd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
  if (fd < 0) throw std::runtime_error("shm_open failed: " + name);

  if (ftruncate(fd, SHM_SIZE) != 0) throw std::runtime_error("ftruncate failed: " + name);

  void* mapped = mmap(nullptr, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mapped == MAP_FAILED) throw std::runtime_error("mmap failed: " + name);

  base = static_cast<Packet*>(mapped);

  if (clear) {
    std::memset(base, 0, SHM_SIZE);
    base->flags.store(0, std::memory_order_relaxed);
  }
}

SharedMemorySegment::~SharedMemorySegment() {
  if (base) munmap(base, SHM_SIZE);
  if (fd >= 0) close(fd);
  if (unlink_on_destroy) { shm_unlink(name.c_str()); }
}

uint32_t SharedMemorySegment::flags() const { return base->flags.load(std::memory_order_acquire); }

void SharedMemorySegment::writeFlags(uint32_t v) { base->flags.store(v, std::memory_order_release); }

void SharedMemorySegment::setFlag(int bit) { base->flags.fetch_or(1u << bit, std::memory_order_acq_rel); }

void SharedMemorySegment::clearFlag(int bit) { base->flags.fetch_and(~(1u << bit), std::memory_order_acq_rel); }

bool SharedMemorySegment::isFlagSet(int bit) const { return (flags() >> bit) & 1u; }

// ------------------------- Thread Helper Functions -----------------------
void logLine(const std::string& s) {
  std::lock_guard<std::mutex> lk(g_log_mtx);
  std::cout << s << std::endl;
}