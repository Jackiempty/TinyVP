// shm/shm.cpp
#include "shm.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>  // for std::min
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <thread>

// ------------------------- Protocol / layout constants -------------------------
namespace {
constexpr int PACKET_VEC_SIZE_OFFSET = 1;
constexpr int PACKET_DATA_OFFSET     = 2;

constexpr std::size_t SHM_SIZE = 8192;  // bytes

constexpr int READY_BIT = 31;
constexpr int VALID_BIT = 30;
constexpr int ACK_BIT   = 29;

std::mutex g_log_mtx;
}  // namespace

uint32_t getVectorSize(const uint32_t* p) { return p[PACKET_VEC_SIZE_OFFSET]; }

void setVectorSize(uint32_t* p, uint32_t size) {
  if (size > MAX_VEC_SIZE) size = MAX_VEC_SIZE;
  p[PACKET_VEC_SIZE_OFFSET] = size;
}

int32_t getData(const uint32_t* p, int vec_id, std::size_t index) {
  int offset = PACKET_DATA_OFFSET + (vec_id * MAX_VEC_SIZE) + index;
  return static_cast<int32_t>(p[offset]);
}

void setData(uint32_t* p, int32_t v, int vec_id, std::size_t index) {
  int offset = PACKET_DATA_OFFSET + (vec_id * MAX_VEC_SIZE) + index;
  p[offset]  = static_cast<uint32_t>(v);
}

void getVectorData(const uint32_t* p, int32_t* dst, int vec_id, uint32_t size) {
  if (size > MAX_VEC_SIZE) size = MAX_VEC_SIZE;
  int offset = PACKET_DATA_OFFSET + (vec_id * MAX_VEC_SIZE);
  std::memcpy(dst, &p[offset], size * sizeof(int32_t));
}

void setVectorData(uint32_t* p, const int32_t* src, int vec_id, uint32_t size) {
  if (size > MAX_VEC_SIZE) size = MAX_VEC_SIZE;
  int offset = PACKET_DATA_OFFSET + (vec_id * MAX_VEC_SIZE);
  std::memcpy(const_cast<uint32_t*>(&p[offset]), src, size * sizeof(int32_t));
}

// ------------------------- Helper Functions (Internal) -------------------------
// Bit manipulation helpers
static inline bool testBit(uint32_t v, int bit) { return (v >> bit) & 1u; }
static inline void setBit(uint32_t& v, int bit) { v |= (1u << bit); }
static inline void clrBit(uint32_t& v, int bit) { v &= ~(1u << bit); }

// Logging helpers
static std::vector<int> packStringToInts(const std::string& msg) {
  std::vector<int> words{};
  char             buffer[32] = {};
  std::memcpy(buffer, msg.data(), std::min<size_t>(msg.size(), sizeof(buffer)));
  for (int i = 0; i < 8; ++i) {
    int value = 0;
    std::memcpy(&value, buffer + i * 4, 4);
    words.push_back(value);
  }
  return words;
}

static std::string unpackIntsToString(const std::vector<int>& words) {
  char      buffer[32] = {};
  const int count      = std::min<int>(8, (int)words.size());
  for (int i = 0; i < count; ++i) { std::memcpy(buffer + i * 4, &words[i], 4); }
  return std::string(buffer, 32);
}

// ------------------------- SharedMemorySegment Implementation -------------------------

SharedMemorySegment::SharedMemorySegment(const std::string& shm_name, bool clear) : name(shm_name) {
  fd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
  if (fd < 0) throw std::runtime_error("shm_open failed: " + name);
  if (ftruncate(fd, SHM_SIZE) != 0) throw std::runtime_error("ftruncate failed: " + name);

  void* mapped = mmap(nullptr, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mapped == MAP_FAILED) throw std::runtime_error("mmap failed: " + name);
  base = static_cast<uint32_t*>(mapped);

  if (clear) std::memset(base, 0, SHM_SIZE);
}

SharedMemorySegment::~SharedMemorySegment() {
  if (base) munmap(base, SHM_SIZE);
  if (fd >= 0) close(fd);
}

uint32_t SharedMemorySegment::flags() const { return base[0]; }
void     SharedMemorySegment::writeFlags(uint32_t v) { base[0] = v; }

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
