// shm/shm.hpp
#ifndef SHM_HPP
#define SHM_HPP

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

constexpr uint32_t MAX_VEC_SIZE = 512;

// ------------------------- Packet model -------------------------
struct Packet {
  std::atomic<uint32_t> flags;
  uint32_t              vec_size;
  int32_t               vec_a[MAX_VEC_SIZE];
  int32_t               vec_b[MAX_VEC_SIZE];

  static constexpr int READY_BIT = 31;
  static constexpr int VALID_BIT = 30;
  static constexpr int ACK_BIT   = 29;
};

// ------------------------- API Functions -------------------------
uint32_t getVectorSize(const Packet* p);
void     setVectorSize(Packet* p, uint32_t size);

// vec_id: 0 for A & C, 1 for B
int32_t getData(const Packet* p, int vec_id, std::size_t index);
void    setData(Packet* p, int32_t v, int vec_id, std::size_t index);

void getVectorData(const Packet* p, int32_t* dst, int vec_id, uint32_t size);
void setVectorData(Packet* p, const int32_t* src, int vec_id, uint32_t size);

// ------------------------- SHM RAII wrapper -------------------------
struct SharedMemorySegment {
  std::string name;
  int         fd   = -1;
  Packet*     base = nullptr;
  bool        unlink_on_destroy;

  static constexpr std::size_t SHM_SIZE = sizeof(Packet);

  explicit SharedMemorySegment(const std::string& shm_name, bool clear = true, bool unlink = false);
  ~SharedMemorySegment();

  uint32_t flags() const;
  void     writeFlags(uint32_t v);
  void     setFlag(int bit);
  void     clearFlag(int bit);
  bool     isFlagSet(int bit) const;
};

// ----------------------- Thread Helper Functions ---------------------
void logLine(const std::string& s);

#endif  // SHM_HPP