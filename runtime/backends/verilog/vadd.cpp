// runtime/backends/verilog/vadd.cpp
#include "vadd.hpp"

RTLBackend::RTLBackend() : shm_h2a("shm_h2a", true), shm_a2h("shm_a2h", true) { logLine("[Host] SHM Initialized."); }

void RTLBackend::vadd(const int32_t* vec_a, const int32_t* vec_b, int32_t* vec_c, uint32_t vec_size) {
  auto* req_ptr  = shm_h2a.base;
  auto* resp_ptr = shm_a2h.base;

  // ---------------------------------------------------------
  // Phase 1: Send Request (Host to Accelerator)
  // ---------------------------------------------------------

  // Wait for Accelerator READY
  while (!shm_h2a.isFlagSet(Packet::READY_BIT)) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }

  // Serialize metadata and payload
  setVectorSize(req_ptr, vec_size);
  setVectorData(req_ptr, vec_a, 0, vec_size);
  setVectorData(req_ptr, vec_b, 1, vec_size);

  // Assert VALID to indicate payload is ready
  shm_h2a.setFlag(Packet::VALID_BIT);

  // Wait for Accelerator ACK
  while (!shm_h2a.isFlagSet(Packet::ACK_BIT)) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }

  // Clear flags to complete transaction
  shm_h2a.clearFlag(Packet::ACK_BIT);
  shm_h2a.clearFlag(Packet::VALID_BIT);

  // ---------------------------------------------------------
  // Phase 2: Receive Response (Accelerator to Host)
  // ---------------------------------------------------------

  // Assert READY to receive result
  shm_a2h.setFlag(Packet::READY_BIT);

  // Wait for Accelerator VALID
  while (!shm_a2h.isFlagSet(Packet::VALID_BIT)) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }

  // Clear READY during data transfer
  shm_a2h.clearFlag(Packet::READY_BIT);

  // Deserialize result payload
  getVectorData(resp_ptr, vec_c, 0, vec_size);

  // Assert ACK to acknowledge receipt
  shm_a2h.setFlag(Packet::ACK_BIT);

  // Wait for Accelerator to clear ACK for synchronization
  while (shm_a2h.isFlagSet(Packet::ACK_BIT)) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
}