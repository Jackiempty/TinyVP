// hal/src/hal_main.cpp
#include <iostream>
#include <vector>

#include "shm.hpp"
#include "vector_add_hal.hpp"

static VectorAddHAL hal;

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  VectorAddHAL accelerator;

  SharedMemorySegment shm_h2a("shm_h2a", false);
  SharedMemorySegment shm_a2h("shm_a2h", false);

  auto* req_ptr  = shm_h2a.base;
  auto* resp_ptr = shm_a2h.base;

  while (true) {
    // --- Receive Request (Reader Logic) ---
    // Signal Ready, Wait for VALID
    logLine("[H2A] waiting VALID on '" + shm_h2a.name + "'");
    shm_h2a.setFlag(READY_BIT);
    while (!shm_h2a.isFlagSet(VALID_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    logLine("[H2A] VALID observed on '" + shm_h2a.name + "'");
    uint32_t vec_size = getVectorSize(req_ptr);

    // No longer ready during the transfer
    shm_h2a.clearFlag(READY_BIT);

    // Decode Packet
    int32_t* vec_a = getData(req_ptr, 0);
    int32_t* vec_b = getData(req_ptr, 1);

    // --- Compute ---
    int32_t* vec_c;
    hal.reset();
    hal.compute(vec_a, vec_b, vec_c, vec_size);

    // Signal ACK to the shm_h2a
    shm_h2a.setFlag(ACK_BIT);

    // Wait until shm clears ACK
    logLine("[H2A] waiting ACK clear on '" + shm_h2a.name + "'");
    while (shm_h2a.isFlagSet(ACK_BIT)) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
    logLine("[H2A] ACK cleared on '" + shm_h2a.name + "'");
    logLine("---- HAL Reader complete ----\n");

    // --- Send Response (Writer Logic) ---
    // Wait Host Ready
    logLine("[A2H] waiting READY on '" + shm_a2h.name + "'");
    while (!shm_a2h.isFlagSet(READY_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    logLine("[A2H] READY observed on '" + shm_a2h.name + "'");

    // Serialize Result, write result back to *vec_a
    setData(resp_ptr, *vec_c, 0);

    // Signal VALID and wait for ACK
    shm_a2h.setFlag(VALID_BIT);
    logLine("[A2H] waiting ACK on '" + shm.name + "'");
    while (!shm_a2h.isFlagSet(ACK_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Clear both flags
    shm_a2h.clearFlag(ACK_BIT);
    shm_a2h.clearFlag(VALID_BIT);
    logLine("[A2H] ACK received, VALID cleared on '" + shm.name + "'");
    logLine("---- HAL Writer complete ----\n");
  }

  return 0;
}