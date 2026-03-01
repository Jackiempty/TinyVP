// csrc/verilog/vadd.hpp
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

#include "common/shm/shm.hpp"

/**
 * @brief Dispatches vector addition workloads to the RTL backend via shared memory.
 *
 * This function utilizes an Instruction-Driven Architecture over a single Shared
 * Memory (SHM) segment. It acts as a memory allocator, copies tensor data into
 * the SHM payload area, pushes a computation command to the lock-free ring buffer,
 * and polls for the completion status asynchronously.
 *
 * @param vec_a    Pointer to the first input vector (int32_t).
 * @param vec_b    Pointer to the second input vector (int32_t).
 * @param vec_c    Pointer to the pre-allocated output vector (int32_t).
 * @param vec_size Number of elements in the vectors.
 */
void vadd(const int32_t* vec_a, const int32_t* vec_b, int32_t* vec_c, uint32_t vec_size);
