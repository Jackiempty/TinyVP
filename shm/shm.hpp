// shm/shm.hpp
#ifndef SHM_HPP
#define SHM_HPP

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

constexpr uint32_t MAX_VEC_SIZE = 512;

// ------------------------- Packet model -------------------------
struct Packet {
  uint32_t vec_size;
  int32_t  vec_a[MAX_VEC_SIZE];
  int32_t  vec_b[MAX_VEC_SIZE];
};

// ------------------------- API Functions -------------------------
uint32_t getVectorSize(const uint32_t* p);
void     setVectorSize(uint32_t* p, uint32_t size);

// vec_id: 0 for A & C, 1 for B
int32_t getData(const uint32_t* p, int vec_id, std::size_t index);
void    setData(uint32_t* p, int32_t v, int vec_id, std::size_t index);

void getVectorData(const uint32_t* p, int32_t* dst, int vec_id, uint32_t size);
void setVectorData(uint32_t* p, const int32_t* src, int vec_id, uint32_t size);

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

#endif  // SHM_HPP