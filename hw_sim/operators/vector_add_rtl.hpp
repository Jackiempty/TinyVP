// hw_sim/operators/vector_add_rtl.hpp
#pragma once

#ifndef VECTOR_ADD_HAL_HPP
#define VECTOR_ADD_HAL_HPP

#include <verilated.h>

#include <iostream>
#include <vector>

#include "Vvector_add.h"

class VectorAddHAL {
  private:
  Vvector_add* dut;
  vluint64_t   main_time = 0;
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

#endif  // VECTOR_ADD_HAL_HPP