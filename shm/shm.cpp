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
constexpr int HEADER_PACKET_ID_INDEX = 1;
constexpr int PACKET_ADDR_INDEX      = 2;
constexpr int PACKET_DATA_OFFSET     = 4;

constexpr std::size_t SHM_SIZE = 8192;  // bytes

constexpr int READY_BIT = 31;
constexpr int VALID_BIT = 30;
constexpr int ACK_BIT   = 29;

std::mutex g_log_mtx;
}  // namespace

// ------------------------- Helper Functions (Internal) -------------------------

static inline uint32_t getPacketId(const uint32_t* p) { return *(p + HEADER_PACKET_ID_INDEX); }
static inline void     setPacketId(uint32_t* p, uint32_t id) { *(p + HEADER_PACKET_ID_INDEX) = id; }

static inline uint64_t getAddr(const uint32_t* p) {
  uint64_t addr = 0;
  std::memcpy(&addr, p + PACKET_ADDR_INDEX, sizeof(addr));
  return addr;
}

static inline void setAddr(uint32_t* p, uint64_t a) { std::memcpy(p + PACKET_ADDR_INDEX, &a, sizeof(a)); }

static inline uint64_t getData(const uint32_t* p, std::size_t j) {
  return *(p + PACKET_DATA_OFFSET + 2 * static_cast<int>(j));
}

static inline void setDataLong(uint32_t* p, uint64_t v, int data_index) {
  *(p + PACKET_DATA_OFFSET + 2 * data_index) = static_cast<uint64_t>(v);
}

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

static std::string packetToString(const Packet& pkt, const std::string& tag) {
  std::ostringstream oss;
  oss << "[" << tag << "] " << " id=" << pkt.packet_id;

  oss << "\n  - payload: " << "addr=0x" << std::hex << std::setw(8) << std::setfill('0') << pkt.addr;

  oss << " data=[";
  const auto& dp      = pkt.data;
  std::string message = unpackIntsToString(dp);
  oss << message;
  oss << "]";

  return oss.str();
}

static Packet makePacket(uint32_t seq) {
  static std::mt19937_64                  gen{std::random_device{}()};
  std::uniform_int_distribution<uint64_t> addr_d(0x1000, 0xFFFFF0);
  Packet                                  pkt{};

  std::string message = {};
  {
    std::lock_guard<std::mutex> lk(g_log_mtx);
    std::cout << "Enter a message: " << std::flush;
  }
  std::getline(std::cin, message);

  std::vector<int> dataWord = packStringToInts(message);
  // std::cout << "finish enter\n"; // Removed distinct logging for cleaner output

  pkt.packet_id = seq;
  pkt.addr      = (addr_d(gen) & ~0xFULL);  // 16B align for readability
  pkt.data.resize(dataWord.size());
  for (int j = 0; j < 8; ++j) pkt.data[j] = dataWord[j];

  return pkt;
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

// ------------------------- Thread Implementation -------------------------

void writerThread(SharedMemorySegment& shm, std::mutex& /*qmtx*/) {
  auto* ptr   = shm.base;
  int   count = 0;
  while (true) {
    // make Packet each round
    Packet pkt = makePacket(count);
    count++;

    // Wait until the receiver is READY
    logLine("[Writer] waiting READY on '" + shm.name + "'");
    while (!shm.isFlagSet(READY_BIT)) { std::this_thread::sleep_for(std::chrono::milliseconds(500)); }
    logLine("[Writer] READY observed on '" + shm.name + "'");

    // Serialize header
    setPacketId(ptr, pkt.packet_id);

    // Serialize payloads and data
    setAddr(ptr, pkt.addr);
    for (int j = 0; j < 8; ++j) {
      const int value = pkt.data[j];
      setDataWord(ptr, value, j);
    }

    // Signal VALID and wait for ACK
    shm.setFlag(VALID_BIT);
    logLine(packetToString(pkt, "Writer Sent (VALID set)"));
    logLine("[Writer] waiting ACK on '" + shm.name + "'");
    while (!shm.isFlagSet(ACK_BIT)) { std::this_thread::sleep_for(std::chrono::milliseconds(500)); }

    // Clear both flags
    shm.clearFlag(ACK_BIT);
    shm.clearFlag(VALID_BIT);
    logLine("[Writer] ACK received, VALID cleared on '" + shm.name + "'");
    logLine("---- Writer cycle complete ----\n");
    // Small delay to avoid busy looping too fast
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

void readerThread(SharedMemorySegment& shm, std::mutex& /*qmtx*/) {
  auto* ptr = shm.base;

  while (true) {
    // Wait for VALID
    logLine("[Reader] waiting VALID on '" + shm.name + "'");
    while (!shm.isFlagSet(VALID_BIT)) {
      shm.setFlag(READY_BIT);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    logLine("[Reader] VALID observed on '" + shm.name + "'");

    // Read header
    Packet pkt{};
    pkt.packet_id = getPacketId(ptr);

    // No longer ready during the transfer
    shm.clearFlag(READY_BIT);

    // Read payloads and data
    pkt.addr = getAddr(ptr);
    std::vector<int> data;
    for (int j = 0; j < 8; ++j) { data.push_back(static_cast<int>(getData(ptr, j))); }
    pkt.data = data;

    // Signal ACK to the shm
    shm.setFlag(ACK_BIT);
    logLine(packetToString(pkt, "Reader Received (ACK set)"));

    // Wait until shm clears ACK
    logLine("[Reader] waiting ACK clear on '" + shm.name + "'");
    while (shm.isFlagSet(ACK_BIT)) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
    logLine("[Reader] ACK cleared on '" + shm.name + "'");
    logLine("---- Reader cycle complete ----\n");
    // Small delay to avoid busy looping too fast
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}