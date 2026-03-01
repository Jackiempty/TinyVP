// hal/ipc/ipc.cpp
#include "ipc.hpp"

#include <cstring>
#include <iostream>
#include <stdexcept>

namespace hal {
namespace ipc {

IpcDevice::IpcDevice() : shm_("aisrt_shm", false), current_offset_(0) {}

dev_addr_t IpcDevice::allocate(size_t size) {
  std::lock_guard<std::mutex> lock(alloc_mutex_);
  dev_addr_t                  addr = current_offset_;
  if (addr + size > PAYLOAD_CAPACITY) {
    current_offset_ = 0;
    addr            = 0;
  }
  current_offset_ += size;
  return addr;
}

void IpcDevice::free(dev_addr_t addr) {
  // Reserved
}

void IpcDevice::copy_to_device(dev_addr_t dst, const void* src, size_t size) {
  std::memcpy(&shm_.layout->payload[dst], src, size);
}

void IpcDevice::copy_from_device(void* dst, dev_addr_t src, size_t size) {
  std::memcpy(dst, &shm_.layout->payload[src], size);
}

void IpcDevice::submit_vadd(dev_addr_t src1, dev_addr_t src2, dev_addr_t dst, uint32_t size) {
  // 1. Construct Command
  Command cmd;
  cmd.opcode      = Opcode::VADD;
  cmd.status      = CmdStatus::PENDING;
  cmd.size        = size;
  cmd.src1_offset = src1;
  cmd.src2_offset = src2;
  cmd.dst_offset  = dst;

  // 2. Put into Ring Buffer
  uint32_t cmd_idx = shm_.push_command(cmd);

  // 3. Wait for hardware execution to complete
  shm_.wait_for_command_done(cmd_idx);

  // 4. Release status
  shm_.layout->cmd_queue[cmd_idx].opcode = Opcode::NOP;
}

}  // namespace ipc
}  // namespace hal
