// hw_sim/operators/vector_add_rtl.hpp
#pragma once

#include <cstddef>
#include <cstdint>

class Vvector_add;

class VectorAddHAL {
  private:
  Vvector_add* dut;
  uint64_t     main_time = 0;
  const int    VEC_LEN   = 4;

  void tick();

  public:
  VectorAddHAL();
  ~VectorAddHAL();

  /**
   * @brief Reset all elements
   */
  void reset();

  /**
   * @brief Compute the outcome of vector addition.
   * @param a Vector a
   * @param b Vector b
   * @return Vector c = a + b
   */
  void compute(const int32_t* a, const int32_t* b, int32_t* result, size_t total_size);
};
