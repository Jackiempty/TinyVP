// hal/ipc/ipc.cpp
#include "ipc.hpp"

#include <cstring>
#include <iostream>

namespace hal {
namespace ipc {

IpcDevice::IpcDevice() : shm_("aisrt_shm", false), head_(0), tail_(0) {}

dev_addr_t IpcDevice::allocate(size_t size) {
  std::lock_guard<std::mutex> lock(alloc_mutex_);

  uint32_t align_mask   = 63;
  uint32_t aligned_size = (size + align_mask) & ~align_mask;

  dev_addr_t assigned_addr = (dev_addr_t)-1;

  if (head_ >= tail_) {
    uint32_t space_at_end = PAYLOAD_CAPACITY - head_;

    if (aligned_size <= space_at_end) {
      assigned_addr = head_;
      head_ += aligned_size;
    } else if (aligned_size <= tail_) {
      assigned_addr = 0;
      head_         = aligned_size;
    }
  } else {
    uint32_t space_between = tail_ - head_;
    if (aligned_size <= space_between) {
      assigned_addr = head_;
      head_ += aligned_size;
    }
  }

  if (assigned_addr == (dev_addr_t)-1) { throw std::bad_alloc(); }

  active_allocs_.push_back({assigned_addr, aligned_size, false});
  return assigned_addr;
}

void IpcDevice::free(dev_addr_t addr) {
  std::lock_guard<std::mutex> lock(alloc_mutex_);

  for (auto& record : active_allocs_) {
    if (record.addr == addr) {
      record.is_freed = true;
      break;
    }
  }

  while (!active_allocs_.empty() && active_allocs_.front().is_freed) {
    auto& oldest_record = active_allocs_.front();

    tail_ = oldest_record.addr + oldest_record.size;
    active_allocs_.pop_front();

    if (active_allocs_.empty()) {
      head_ = 0;
      tail_ = 0;
    }
  }
}

void IpcDevice::copy_to_device(dev_addr_t dst, const void* src, size_t size) {
  if (dst + size > PAYLOAD_CAPACITY) { throw std::out_of_range("IPC copy_to_device: out of bounds"); }
  std::memcpy(&shm_.layout->payload[dst], src, size);
}

void IpcDevice::copy_from_device(void* dst, dev_addr_t src, size_t size) {
  if (src + size > PAYLOAD_CAPACITY) { throw std::out_of_range("IPC copy_from_device: out of bounds"); }
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
