// shm/shm.hpp
#ifndef SHM_HPP
#define SHM_HPP

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

// ------------------------- Packet model -------------------------
struct Packet {
  uint32_t  vec_size = 0;
  int32_t** vectors  = NULL;
};

uint32_t getVectorSize(const uint32_t* p);
void     setVectorSize(uint32_t* p, uint32_t size);

uint64_t getData(const uint32_t* p, std::size_t j);
void     setData(uint32_t* p, int32_t v, int data_index);

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