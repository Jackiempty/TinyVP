// common/shm/types.hpp
#pragma once
#include <cstdint>

// Opcodes
enum class Opcode : uint32_t {
  NOP  = 0,
  VADD = 1,  // vector addition
  // MATMUL = 2,   // preserved for future
  FINISH = 99  // shutdown HAL server
};

// instruction execution status
enum class CmdStatus : uint32_t {
  PENDING = 0,  // Host push in instruction, waiting for HAL to fetch
  RUNNING = 1,  // HAL processing...
  DONE    = 2   // HAL finish processing, outcome push back to Payload
};

// Command Descriptor
struct Command {
  Opcode             opcode;
  volatile CmdStatus status;  // Use volitale to ensure each R/W goes through Cache

  uint32_t size;         // Vector Length
  uint32_t src1_offset;  // Index 1 offset inside SHM payload
  uint32_t src2_offset;  // Index 2 offset inside SHM payload
  uint32_t dst_offset;   // destination offset inside SHM payload
};
