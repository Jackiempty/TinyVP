// csrc/device/vadd/vadd.cpp
#include "vadd.hpp"

#include "hal/hal.hpp"

void vadd(const int32_t* vec_a, const int32_t* vec_b, int32_t* vec_c, uint32_t size) {
  auto     dev       = hal::get_device();
  uint32_t byte_size = size * sizeof(int32_t);

  // ==========================================
  // Step 1: Memory Allocation
  // ==========================================
  auto addr_a = dev->allocate(byte_size);
  auto addr_b = dev->allocate(byte_size);
  auto addr_c = dev->allocate(byte_size);

  // ==========================================
  // Step 2: Data Transfer (Host to Device)
  // ==========================================
  dev->copy_to_device(addr_a, vec_a, byte_size);
  dev->copy_to_device(addr_b, vec_b, byte_size);

  // ==========================================
  // Step 3 & 4: Instruction Dispatch & Wait
  // ==========================================
  dev->submit_vadd(addr_a, addr_b, addr_c, size);

  // ==========================================
  // Step 5: Readback (Device to Host)
  // ==========================================
  dev->copy_from_device(vec_c, addr_c, byte_size);
}