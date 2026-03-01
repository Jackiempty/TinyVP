// hal/ipc/ipc.hpp
#pragma once
#include <mutex>

#include "common/shm/shm.hpp"
#include "hal/hal.hpp"

namespace hal {
namespace ipc {

class IpcDevice : public IDevice {
  private:
  SharedMemorySegment shm_;

  uint32_t   current_offset_;
  std::mutex alloc_mutex_;

  public:
  IpcDevice();
  ~IpcDevice() override = default;

  dev_addr_t allocate(size_t size) override;
  void       free(dev_addr_t addr) override;

  void copy_to_device(dev_addr_t dst, const void* src, size_t size) override;
  void copy_from_device(void* dst, dev_addr_t src, size_t size) override;

  void submit_vadd(dev_addr_t src1, dev_addr_t src2, dev_addr_t dst, uint32_t size) override;
};

}  // namespace ipc
}  // namespace hal
