// hal/hal.hpp
#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace hal {

// Definition of the device memory address (Opaque handle)
using dev_addr_t = uint32_t;

// HAL interface
class IDevice {
  public:
  virtual ~IDevice() = default;

  // ==========================================
  // Memory Management API
  // ==========================================
  virtual dev_addr_t allocate(size_t size) = 0;
  virtual void       free(dev_addr_t addr) = 0;

  // ==========================================
  // Data Passing API
  // ==========================================
  virtual void copy_to_device(dev_addr_t dst, const void* src, size_t size) = 0;
  virtual void copy_from_device(void* dst, dev_addr_t src, size_t size)     = 0;

  // ==========================================
  // Operator Dispatch API
  // ==========================================
  virtual void submit_vadd(dev_addr_t src1, dev_addr_t src2, dev_addr_t dst, uint32_t size) = 0;
};

// Factory Function：Get the actual device (Return IPC or Driver according to the environment variable)
std::shared_ptr<IDevice> get_device();
}  // namespace hal
