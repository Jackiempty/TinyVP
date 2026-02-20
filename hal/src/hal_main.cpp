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
    // ---------------------------------------------------------
    // Phase 1: Receive Request (Host to Accelerator)
    // ---------------------------------------------------------

    // Assert READY and wait for VALID
    logLine("[H2A] waiting VALID on '" + shm_h2a.name + "'");
    shm_h2a.setFlag(Packet::READY_BIT);
    while (!shm_h2a.isFlagSet(Packet::VALID_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    logLine("[H2A] VALID observed on '" + shm_h2a.name + "'");

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

    // ---------------------------------------------------------
    // Phase 2: Hardware Computation
    // ---------------------------------------------------------
    hal.reset();
    hal.compute(vec_a, vec_b, vec_c, vec_size);

    // Wait for Host to clear ACK
    logLine("[H2A] waiting ACK clear on '" + shm_h2a.name + "'");
    while (shm_h2a.isFlagSet(Packet::ACK_BIT)) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
    logLine("[H2A] ACK cleared on '" + shm_h2a.name + "'");
    logLine("---- HAL Reader complete ----\n");

    // ---------------------------------------------------------
    // Phase 3: Send Response (Accelerator to Host)
    // ---------------------------------------------------------

    // Wait for Host READY
    logLine("[A2H] waiting READY on '" + shm_a2h.name + "'");
    while (!shm_a2h.isFlagSet(Packet::READY_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    logLine("[A2H] READY observed on '" + shm_a2h.name + "'");

    // Serialize result payload
    setVectorSize(resp_ptr, vec_size);
    setVectorData(resp_ptr, vec_c, 0, vec_size);

    // Assert VALID and wait for ACK
    shm_a2h.setFlag(Packet::VALID_BIT);
    logLine("[A2H] waiting ACK on '" + shm_a2h.name + "'");
    while (!shm_a2h.isFlagSet(Packet::ACK_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Clear flags to complete transaction
    shm_a2h.clearFlag(Packet::ACK_BIT);
    shm_a2h.clearFlag(Packet::VALID_BIT);
    logLine("[A2H] ACK received, VALID cleared on '" + shm_a2h.name + "'");
    logLine("---- HAL Writer complete ----\n");
  }

  return 0;
}