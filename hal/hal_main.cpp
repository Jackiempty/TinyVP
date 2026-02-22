// hal/src/hal_main.cpp
#include <iostream>
#include <vector>

#include "shm.hpp"
#include "vector_add_hal.hpp"

#ifndef DEBUG_SHM
#define DEBUG_SHM 0
#endif

#define SHM_LOG(msg) \
  if (DEBUG_SHM) { logLine(msg); }

static VectorAddHAL hal;

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  VectorAddHAL accelerator;

  SharedMemorySegment shm_h2a("shm_h2a", true, true);
  SharedMemorySegment shm_a2h("shm_a2h", true, true);

  auto* req_ptr  = shm_h2a.base;
  auto* resp_ptr = shm_a2h.base;

  while (true) {
    // ---------------------------------------------------------
    // Phase 1: Receive Request (Host to Accelerator)
    // ---------------------------------------------------------

    // Assert READY and wait for VALID
    SHM_LOG("[H2A] [HAL] waiting VALID...");
    shm_h2a.setFlag(Packet::READY_BIT);
    while (!shm_h2a.isFlagSet(Packet::VALID_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    SHM_LOG("[H2A] [HAL] VALID observed");

    // Clear READY during data transfer
    shm_h2a.clearFlag(Packet::READY_BIT);

    // Deserialize payload
    uint32_t vec_size = getVectorSize(req_ptr);
    int32_t  vec_a[MAX_VEC_SIZE];
    int32_t  vec_b[MAX_VEC_SIZE];
    int32_t  vec_c[MAX_VEC_SIZE];
    getVectorData(req_ptr, vec_a, 0, vec_size);
    getVectorData(req_ptr, vec_b, 1, vec_size);

    // Assert ACK to acknowledge receipt
    shm_h2a.setFlag(Packet::ACK_BIT);
    logLine("[HAL] Received Vector Add Request (Size: " + std::to_string(vec_size) + ")");

    // ---------------------------------------------------------
    // Phase 2: Hardware Computation
    // ---------------------------------------------------------
    hal.reset();
    hal.compute(vec_a, vec_b, vec_c, vec_size);
    logLine("[HAL] Hardware Computation Completed.");

    // Wait for Host to clear ACK
    SHM_LOG("[H2A] [HAL] waiting ACK clear");
    while (shm_h2a.isFlagSet(Packet::ACK_BIT)) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    SHM_LOG("---- HAL Reader complete ----");

    // ---------------------------------------------------------
    // Phase 3: Send Response (Accelerator to Host)
    // ---------------------------------------------------------
    // Wait for Host READY
    SHM_LOG("[A2H] [HAL] waiting READY");
    while (!shm_a2h.isFlagSet(Packet::READY_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Serialize result payload
    setVectorSize(resp_ptr, vec_size);
    setVectorData(resp_ptr, vec_c, 0, vec_size);

    // Assert VALID and wait for ACK
    shm_a2h.setFlag(Packet::VALID_BIT);
    SHM_LOG("[A2H] [HAL] waiting ACK");
    while (!shm_a2h.isFlagSet(Packet::ACK_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Clear flags to complete transaction
    shm_a2h.clearFlag(Packet::ACK_BIT);
    shm_a2h.clearFlag(Packet::VALID_BIT);
    logLine("[HAL] Result Dispatched to Host.");
    SHM_LOG("---- HAL Writer complete ----\n");
  }

  return 0;
}
