#include "shm.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>  // for std::min
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>

static std::mutex g_log_mtx;

// ------------------------- Helpers Implementation -------------------------

std::vector<int> packStringToInts(const std::string& msg) {
  std::vector<int> words{};
  char             buffer[32] = {};
  // copy string content（at most 32 bytes）
  std::memcpy(buffer, msg.data(), std::min<size_t>(msg.size(), sizeof(buffer)));
  // pack every 4 bytes into an int
  for (int i = 0; i < 8; ++i) {
    int value = 0;
    std::memcpy(&value, buffer + i * 4, 4);
    words.push_back(value);
  }
  return words;
}

std::string unpackIntsToString(const std::vector<int>& words) {
  char      buffer[32] = {};
  const int count      = std::min<int>(8, (int)words.size());
  for (int i = 0; i < count; ++i) { std::memcpy(buffer + i * 4, &words[i], 4); }
  return std::string(buffer, 32);
}

void logLine(const std::string& s) {
  std::lock_guard<std::mutex> lk(g_log_mtx);
  std::cout << s << std::endl;
}

std::string packetToString(const Packet& pkt, const std::string& tag) {
  std::ostringstream oss;
  oss << "[" << tag << "] " << " id=" << pkt.packet_id << " payloads=" << pkt.payload.size();

  for (size_t i = 0; i < pkt.payload.size(); ++i) {
    const auto& pl = pkt.payload[i];
    oss << "\n  - payload[" << i << "]: " << "addr=0x" << std::hex << std::setw(8) << std::setfill('0') << pl.addr
        << std::dec << " read=" << pl.read << " flit_words=" << pl.flit_word_num;

    if (/*pl.read == 1 &&*/ i < pkt.data_payload.size()) {
      oss << " data=[";
      const auto& dp      = pkt.data_payload[i].data;
      std::string message = unpackIntsToString(dp);
      oss << message;
      oss << "]";
    }
  }
  return oss.str();
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

// ------------------------- Internal Logic -------------------------

static Packet makePacket(uint32_t seq) {
  static std::mt19937_64                  gen{std::random_device{}()};
  std::uniform_int_distribution<int>      payload_cnt_d(1, 4);
  std::uniform_int_distribution<int>      rw_d(0, 1);
  std::uniform_int_distribution<uint64_t> addr_d(0x1000, 0xFFFFF0);

  Packet pkt{};

  // Use iostream to fetch the message and pack it to be sent by writer
  std::string message = {};
  std::cout << "Enter a message: ";  // 稍微修改提示以便閱讀
  std::getline(std::cin, message);
  std::vector<int> dataWord = packStringToInts(message);
  // std::cout << "finish enter\n";

  pkt.packet_id = seq;
  pkt.payload.reserve(1);
  pkt.data_payload.reserve(1);

  // generate one payload and message
  Payload pl{};
  pl.addr          = (addr_d(gen) & ~0xFULL);  // 16B align for readability
  pl.read          = rw_d(gen);
  pl.flit_word_num = (int)dataWord.size();
  pkt.payload.push_back(pl);

  DataPayload dp;
  dp.data.resize(pl.flit_word_num);
  for (int j = 0; j < pl.flit_word_num; ++j) dp.data[j] = dataWord[j];
  pkt.data_payload.push_back(std::move(dp));

  return pkt;
}

// ------------------------- Worker Threads Implementation -------------------------

void writerThread(SharedMemorySegment& shm, std::queue<Packet>& q, std::mutex& qmtx) {
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
    setPayloadCount(ptr, static_cast<uint32_t>(pkt.payload.size()));

    // Serialize payloads and data
    for (int i = 0; i < static_cast<int>(pkt.payload.size()); ++i) {
      setAddr(ptr, pkt.payload[i].addr, i);
      // Clamp metadata so it always matches the serialized layout
      int read_flag       = (pkt.payload[i].read != 0) ? 1 : 0;
      pkt.payload[i].read = read_flag;
      setReq(ptr, read_flag, i);

      int flit_words = pkt.payload[i].flit_word_num;
      if (flit_words < 0) flit_words = 0;
      if (flit_words > MAX_FLIT_WORDS) flit_words = MAX_FLIT_WORDS;
      pkt.payload[i].flit_word_num = flit_words;
      setFlit(ptr, flit_words, i);

      DataPayload* dp        = (i < static_cast<int>(pkt.data_payload.size())) ? &pkt.data_payload[i] : nullptr;
      const int    data_size = dp ? static_cast<int>(dp->data.size()) : 0;
      if (dp) {
        if (data_size > flit_words) {
          dp->data.resize(flit_words);
        } else if (data_size < flit_words) {
          dp->data.resize(flit_words, 0);
        }
      }

      for (int j = 0; j < flit_words; ++j) {
        const int value = (dp && j < static_cast<int>(dp->data.size())) ? dp->data[j] : 0;
        setDataWord(ptr, value, i, j);
      }
      // Clear any unused data slots to prevent stale values on the reader
      for (int j = flit_words; j < MAX_FLIT_WORDS; ++j) { setDataWord(ptr, 0, i, j); }
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

void readerThread(SharedMemorySegment& shm, std::queue<Packet>& q, std::mutex& qmtx) {
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
    pkt.packet_id     = getPacketId(ptr);
    int payload_count = static_cast<int>(getPayloadCount(ptr));
    if (payload_count < 0) payload_count = 0;
    const int total_words       = static_cast<int>(SHM_SIZE / sizeof(uint32_t));
    const int max_payload_slots = (total_words - PAYLOAD_BASE_INDEX) / PAYLOAD_STRIDE_WORDS;
    if (payload_count > max_payload_slots) payload_count = max_payload_slots;

    // No longer ready during the transfer
    shm.clearFlag(READY_BIT);

    // Read payloads and data
    pkt.payload.reserve(payload_count);
    pkt.data_payload.reserve(payload_count);
    for (int i = 0; i < payload_count; ++i) {
      const uint64_t a = getAddr(ptr, i);
      Payload        pl{};
      pl.addr = a;
      pl.read = (getReq(ptr, i) != 0) ? 1 : 0;

      int flit_words = static_cast<int>(getFlit(ptr, i));
      if (flit_words < 0) flit_words = 0;
      if (flit_words > MAX_FLIT_WORDS) flit_words = MAX_FLIT_WORDS;
      pl.flit_word_num = flit_words;
      pkt.payload.push_back(pl);

      DataPayload dp;
      dp.data.reserve(pl.flit_word_num);
      for (int j = 0; j < pl.flit_word_num; ++j) { dp.data.push_back(static_cast<int>(getData(ptr, i, j))); }
      pkt.data_payload.push_back(std::move(dp));
    }

    // Signal ACK to the shm
    shm.setFlag(ACK_BIT);
    logLine(packetToString(pkt, "Reader Received (ACK set)"));

    // Queue the received packet
    // {
    //  std::lock_guard<std::mutex> lk(qmtx);
    //  q.push(std::move(pkt));
    // }

    // Wait until shm clears ACK
    logLine("[Reader] waiting ACK clear on '" + shm.name + "'");
    while (shm.isFlagSet(ACK_BIT)) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
    logLine("[Reader] ACK cleared on '" + shm.name + "'");
    logLine("---- Reader cycle complete ----\n");
    // Small delay to avoid busy looping too fast
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}