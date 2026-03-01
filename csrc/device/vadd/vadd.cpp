// csrc/verilog/vadd.cpp
#include "vadd.hpp"

#ifndef DEBUG_HOST
#define DEBUG_HOST 0
#endif

#if DEBUG_HOST
#define HOST_DEBUG(x) std::cout << "[Host DEBUG] " << x << std::endl
#else
#define HOST_DEBUG(x) \
  do {                \
  } while (0)
#endif

extern SharedMemorySegment shm;

void vadd(const int32_t* vec_a, const int32_t* vec_b, int32_t* vec_c, uint32_t size) {
  // ==========================================
  // Step 1: Memory Allocation
  // ==========================================
  // Because the payload is a huge continuous int32_t array, we need to allocate offsets.
  uint32_t offset_a = 0;
  uint32_t offset_b = offset_a + size;
  uint32_t offset_c = offset_b + size;

  // Failsafe: Ensure it does not exceed the SHM Payload capacity
  if (offset_c + size > PAYLOAD_CAPACITY) { throw std::runtime_error("SHM Payload capacity exceeded!"); }
  HOST_DEBUG("Allocating VADD Payload - Size: " << size << " | a:" << offset_a << " b:" << offset_b
                                                << " c:" << offset_c);

  // ==========================================
  // Step 2: Data Transfer
  // ==========================================
  // Directly copy PyTorch data to the calculated payload addresses
  std::memcpy(&shm.layout->payload[offset_a], vec_a, size * sizeof(int32_t));
  std::memcpy(&shm.layout->payload[offset_b], vec_b, size * sizeof(int32_t));

  // ==========================================
  // Step 3: Instruction Dispatch
  // ==========================================
  Command cmd;
  cmd.opcode      = Opcode::VADD;
  cmd.status      = CmdStatus::PENDING;  // 初始狀態
  cmd.size        = size;
  cmd.src1_offset = offset_a;
  cmd.src2_offset = offset_b;
  cmd.dst_offset  = offset_c;

  // Push the command into the Ring Buffer and get the index (Ticket) of that command
  uint32_t cmd_idx = shm.push_command(cmd);
  HOST_DEBUG("Pushed VADD Command at Queue Index: " << cmd_idx);

  // ==========================================
  // Step 4: Wait for hardware execution to complete (Wait / Poll)
  // ==========================================
  // Call the encapsulated polling API of SHM to hide the underlying status check logic
  shm.wait_for_command_done(cmd_idx);
  HOST_DEBUG("VADD Command " << cmd_idx << " Done. Reading back results.");

  // ==========================================
  // Step 5: Readback
  // ==========================================
  std::memcpy(vec_c, &shm.layout->payload[offset_c], size * sizeof(int32_t));

  // Release the status of this command
  shm.layout->cmd_queue[cmd_idx].opcode = Opcode::NOP;
}
