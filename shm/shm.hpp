// shm/shm.hpp
#ifndef SHM_HPP
#define SHM_HPP

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

// ------------------------- Packet model -------------------------
struct Packet {
  uint32_t              packet_id = 0;
  uint64_t              addr      = 0;
  std::vector<uint64_t> data_addr;
};

// ------------------------- SHM RAII wrapper -------------------------
struct SharedMemorySegment {
  std::string name;
  int         fd   = -1;
  uint32_t*   base = nullptr;

  explicit SharedMemorySegment(const std::string& shm_name, bool clear = true);
  ~SharedMemorySegment();

  uint32_t flags() const;
  void     writeFlags(uint32_t v);
  void     setFlag(int bit);
  void     clearFlag(int bit);
  bool     isFlagSet(int bit) const;
};

// ----------------------- Thread Helper Functions ---------------------
void logLine(const std::string& s);

// ------------------------- Thread Functions -------------------------
void writerThread(SharedMemorySegment& shm, std::mutex& qmtx);
void readerThread(SharedMemorySegment& shm, std::mutex& qmtx);

#endif  // SHM_HPP