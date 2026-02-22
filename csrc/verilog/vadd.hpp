// runtime/backends/verilog/vadd.hpp
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "shm.hpp"

class RTLBackend {
  private:
  SharedMemorySegment shm_h2a;
  SharedMemorySegment shm_a2h;

  public:
  RTLBackend();

  /**
   * @brief Dispatches vector addition workloads to the RTL backend via shared memory.
   *
   * This function manages a two-phase handshake protocol (Request and Response)
   * over two distinct shared memory channels (H2A and A2H). It serializes the input
   * data, synchronizes with the hardware abstraction layer (HAL), and deserializes
   * the computation results.
   *
   * @param vec_a    Pointer to the first input vector (int32_t).
   * @param vec_b    Pointer to the second input vector (int32_t).
   * @param vec_c    Pointer to the pre-allocated output vector (int32_t).
   * @param vec_size Number of elements in the vectors.
   */
  void vadd(const int32_t* vec_a, const int32_t* vec_b, int32_t* vec_c, uint32_t vec_size);
};
