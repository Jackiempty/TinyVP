// hal/hal_main.cpp
#include <verilated.h>

#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "operators/vector_add_hal.hpp"
#include "shm.hpp"

volatile sig_atomic_t keep_running = 1;

void signal_handler(int signum) {
  std::cout << "\n[HAL] Interrupt signal (" << signum << ") received. Initiating graceful shutdown..." << std::endl;
  keep_running = 0;
}

int main(int argc, char** argv) {
  // ==========================================
  // 0. System Environment Initialization
  // ==========================================
  // Pass startup arguments to Verilator (crucial for enabling waveform trace)
  Verilated::commandArgs(argc, argv);

  // Register OS interrupt signals (Ctrl+C or kill)
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  // ==========================================
  // 1. Initialize Communication and Hardware Modules
  // ==========================================
  std::cout << "[HAL] Initializing Shared Memory Segment..." << std::endl;
  SharedMemorySegment shm("aisrt_shm", true);  // Server is responsible for clearing the SHM
  VectorAddHAL        vadd_module;

  std::cout << "[HAL] Dispatcher Started. Waiting for instructions..." << std::endl;

  // ==========================================
  // 2. Core Dispatch Loop (Fetch-Decode-Execute)
  // ==========================================
  while (keep_running) {
    // --- FETCH ---
    if (shm.has_new_command()) {
      Command* cmd = shm.fetch_command();

      // Update status to running. Add compiler barrier to prevent instruction reordering
      cmd->status = CmdStatus::RUNNING;
      asm volatile("" ::: "memory");

      // --- DECODE & EXECUTE ---
      switch (cmd->opcode) {
        case Opcode::VADD: {
          // Safety check: Check if offsets exceed Payload capacity
          if (cmd->dst_offset + cmd->size > PAYLOAD_CAPACITY || cmd->src1_offset + cmd->size > PAYLOAD_CAPACITY) {
            std::cerr << "[HAL] ERROR: Memory boundary exceeded!" << std::endl;
            break;
          }

          // Decode Offsets into physical memory pointers
          int32_t* a = &shm.layout->payload[cmd->src1_offset];
          int32_t* b = &shm.layout->payload[cmd->src2_offset];
          int32_t* c = &shm.layout->payload[cmd->dst_offset];

          // Drive hardware computation
          vadd_module.compute(a, b, c, cmd->size);
          break;
        }

        case Opcode::FINISH: {
          std::cout << "[HAL] FINISH command received. Terminating loop." << std::endl;
          keep_running = 0;  // Trigger loop termination
          break;
        }

        case Opcode::NOP: {
          // NOP command, pass through directly (can be used to test communication latency)
          break;
        }

        default: {
          std::cerr << "[HAL] WARNING: Unknown Opcode (" << static_cast<uint32_t>(cmd->opcode) << ") detected!"
                    << std::endl;
          break;
        }
      }

      // --- WRITEBACK ---
      // Ensure all payload write operations are committed to memory before updating status to DONE
      asm volatile("" ::: "memory");
      cmd->status = CmdStatus::DONE;

    } else {
      // Short sleep when Queue is idle to yield CPU resources
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
  }

  // ==========================================
  // 3. Resource Release
  // ==========================================
  std::cout << "[HAL] Server offline. Goodbye." << std::endl;
  return 0;
}
