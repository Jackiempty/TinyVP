#ifndef SHM_HPP
#define SHM_HPP

#include <cstdint>
#include <cstring>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

// ------------------------- Packet model -------------------------
struct Payload {
  uint64_t addr          = 0;
  int      read          = 0;  // 0: write, 1: read
  int      flit_word_num = 0;  // number of words in this flit
};

struct DataPayload {
  std::vector<int> data;  // up to 8 words/32 chars per packet
};

struct Packet {
  uint32_t                 packet_id = 0;
  std::vector<Payload>     payload;
  std::vector<DataPayload> data_payload;
};

// ------------------------- Protocol / layout constants -------------------------
constexpr int HEADER_PACKET_ID_INDEX     = 1;
constexpr int HEADER_PAYLOAD_COUNT_INDEX = 2;
constexpr int PAYLOAD_BASE_INDEX         = 3;
constexpr int PAYLOAD_STRIDE_WORDS       = 12;
constexpr int PAYLOAD_READ_OFFSET        = 2;
constexpr int PAYLOAD_FLIT_OFFSET        = 3;
constexpr int PAYLOAD_DATA_OFFSET        = 4;
constexpr int MAX_FLIT_WORDS             = 8;

// ------------------------- Inline Layout Accessors -------------------------
inline uint32_t getPacketId(const uint32_t* p) { return *(p + HEADER_PACKET_ID_INDEX); }
inline void     setPacketId(uint32_t* p, uint32_t id) { *(p + HEADER_PACKET_ID_INDEX) = id; }

inline uint32_t getPayloadCount(const uint32_t* p) { return *(p + HEADER_PAYLOAD_COUNT_INDEX); }
inline void     setPayloadCount(uint32_t* p, uint32_t count) { *(p + HEADER_PAYLOAD_COUNT_INDEX) = count; }

inline int payloadBaseWord(std::size_t idx) {
  return PAYLOAD_BASE_INDEX + PAYLOAD_STRIDE_WORDS * static_cast<int>(idx);
}

inline uint64_t getAddr(const uint32_t* p, std::size_t idx) {
  const int base = payloadBaseWord(idx);
  uint64_t  addr = 0;
  std::memcpy(&addr, p + base, sizeof(addr));
  return addr;
}

inline void setAddr(uint32_t* p, uint64_t a, int index) {
  const int base = payloadBaseWord(static_cast<std::size_t>(index));
  std::memcpy(p + base, &a, sizeof(a));
}

inline uint32_t getReq(const uint32_t* p, std::size_t idx) {
  const int base = payloadBaseWord(idx);
  return *(p + base + PAYLOAD_READ_OFFSET);
}

inline void setReq(uint32_t* p, int v, int index) {
  const int base                    = payloadBaseWord(static_cast<std::size_t>(index));
  *(p + base + PAYLOAD_READ_OFFSET) = static_cast<uint32_t>(v);
}

inline uint32_t getFlit(const uint32_t* p, std::size_t idx) {
  const int base = payloadBaseWord(idx);
  return *(p + base + PAYLOAD_FLIT_OFFSET);
}

inline void setFlit(uint32_t* p, int v, int index) {
  const int base                    = payloadBaseWord(static_cast<std::size_t>(index));
  *(p + base + PAYLOAD_FLIT_OFFSET) = static_cast<uint32_t>(v);
}

inline uint32_t getData(const uint32_t* p, std::size_t idx, std::size_t j) {
  const int base = payloadBaseWord(idx);
  return *(p + base + PAYLOAD_DATA_OFFSET + static_cast<int>(j));
}

inline void setDataWord(uint32_t* p, int v, int index, int data_index) {
  const int base                                 = payloadBaseWord(static_cast<std::size_t>(index));
  *(p + base + PAYLOAD_DATA_OFFSET + data_index) = static_cast<uint32_t>(v);
}

// ------------------------- SHM Constants & Utils -------------------------
constexpr std::size_t SHM_SIZE  = 8192;  // bytes
constexpr int         READY_BIT = 31;
constexpr int         VALID_BIT = 30;
constexpr int         ACK_BIT   = 29;

inline bool testBit(uint32_t v, int bit) { return (v >> bit) & 1u; }
inline void setBit(uint32_t& v, int bit) { v |= (1u << bit); }
inline void clrBit(uint32_t& v, int bit) { v &= ~(1u << bit); }

// ------------------------- SharedMemorySegment Class -------------------------
struct SharedMemorySegment {
  std::string name;
  int         fd   = -1;
  uint32_t*   base = nullptr;

  explicit SharedMemorySegment(const std::string& shm_name, bool clear = true);
  ~SharedMemorySegment();

  SharedMemorySegment(const SharedMemorySegment&)            = delete;
  SharedMemorySegment& operator=(const SharedMemorySegment&) = delete;

  uint32_t flags() const;
  void     writeFlags(uint32_t v);
  void     setFlag(int bit);
  void     clearFlag(int bit);
  bool     isFlagSet(int bit) const;
};

// ------------------------- Helper Functions Declarations -------------------------
std::vector<int> packStringToInts(const std::string& msg);
std::string      unpackIntsToString(const std::vector<int>& words);
void             logLine(const std::string& s);
std::string      packetToString(const Packet& pkt, const std::string& tag);

// ------------------------- Thread Workers Declarations -------------------------
void writerThread(SharedMemorySegment& shm, std::queue<Packet>& q, std::mutex& qmtx);
void readerThread(SharedMemorySegment& shm, std::queue<Packet>& q, std::mutex& qmtx);

#endif  // SHM_HPP