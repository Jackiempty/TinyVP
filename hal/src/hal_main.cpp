// hal/src/hal_main.cpp
#include <iostream>
#include <vector>

#include "shm.hpp"
#include "vector_add_hal.hpp"

static VectorAddHAL hal;

// [Yet] Expand Makefile to drive the whole project
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

// [OK] test_main that validate whether the compilation flow works and whether the result is right
/*
int test_main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);

  std::cout << "--- [C++] Starting Simulation ---" << std::endl;
  int32_t host_a[] = {10, 20, 30, 40};
  int32_t host_b[] = {1, 2, 3, 4};
  int32_t host_c[4];
  hal.compute(host_a, host_b, host_c, 4);

  bool pass = true;
  for (int i = 0; i < 4; i++) {
    int32_t expected = host_a[i] + host_b[i];
    std::cout << "Index " << i << ": " << host_a[i] << " + " << host_b[i] << " = " << host_c[i]
              << " (Expected: " << expected << ")" << std::endl;

    if (host_c[i] != expected) pass = false;
  }

  if (pass) {
    std::cout << "--- [PASS] Hardware matches Software! ---" << std::endl;
  } else {
    std::cout << "--- [FAIL] Mismatch detected! ---" << std::endl;
  }

  return 0;
}
*/