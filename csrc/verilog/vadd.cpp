// runtime/backends/verilog/vadd.cpp
#include "vadd.hpp"

#ifndef DEBUG_SHM
#define DEBUG_SHM 0
#endif

#define SHM_LOG(msg) \
  if (DEBUG_SHM) { logLine(msg); }

RTLBackend::RTLBackend() : shm_h2a("shm_h2a", false), shm_a2h("shm_a2h", false) { logLine("[Host] SHM Initialized."); }

void RTLBackend::vadd(const int32_t* vec_a, const int32_t* vec_b, int32_t* vec_c, uint32_t vec_size) {
  auto* req_ptr  = shm_h2a.base;
  auto* resp_ptr = shm_a2h.base;

  // ---------------------------------------------------------
  // Phase 1: Send Request (Host to Accelerator)
  // ---------------------------------------------------------

  // Wait for Accelerator READY
  SHM_LOG("[H2A] [Host] Waiting for Accelerator READY...");
  while (!shm_h2a.isFlagSet(Packet::READY_BIT)) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }

  // Serialize metadata and payload
  SHM_LOG("[H2A] [Host] READY observed. Serializing payload...");
  setVectorSize(req_ptr, vec_size);
  setVectorData(req_ptr, vec_a, 0, vec_size);
  setVectorData(req_ptr, vec_b, 1, vec_size);

  // Assert VALID to indicate payload is ready
  SHM_LOG("[H2A] [Host] Asserting VALID...");
  shm_h2a.setFlag(Packet::VALID_BIT);

  // Wait for Accelerator ACK
  SHM_LOG("[H2A] [Host] Waiting for Accelerator ACK...");
  while (!shm_h2a.isFlagSet(Packet::ACK_BIT)) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }

  // Clear flags to complete transaction
  SHM_LOG("[H2A] [Host] ACK observed. Clearing flags...");
  shm_h2a.clearFlag(Packet::ACK_BIT);
  shm_h2a.clearFlag(Packet::VALID_BIT);

  // ---------------------------------------------------------
  // Phase 2: Receive Response (Accelerator to Host)
  // ---------------------------------------------------------

  // Assert READY to receive result
  SHM_LOG("[A2H] [Host] Asserting READY to receive result...");
  shm_a2h.setFlag(Packet::READY_BIT);

  // Wait for Accelerator VALID
  SHM_LOG("[A2H] [Host] Waiting for Accelerator VALID...");
  while (!shm_a2h.isFlagSet(Packet::VALID_BIT)) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }

  // Clear READY during data transfer
  SHM_LOG("[A2H] [Host] VALID observed. Deserializing result...");
  shm_a2h.clearFlag(Packet::READY_BIT);

  // Deserialize result payload
  SHM_LOG("[A2H] [Host] Asserting ACK...");
  getVectorData(resp_ptr, vec_c, 0, vec_size);

  // Assert ACK to acknowledge receipt
  SHM_LOG("[A2H] [Host] Waiting for Accelerator to clear ACK...");
  shm_a2h.setFlag(Packet::ACK_BIT);

  // Wait for Accelerator to clear ACK for synchronization
  SHM_LOG("[A2H] [Host] Transaction complete.\n");
  while (shm_a2h.isFlagSet(Packet::ACK_BIT)) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
}