// cpp/vector_add_hal.hpp
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
  std::vector<uint32_t> compute(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b);
};

#endif  // VECTOR_ADD_HAL_HPP