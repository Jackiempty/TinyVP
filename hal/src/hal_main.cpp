// cpp/main.cpp
#include <iostream>
#include <vector>

#include "shm.hpp"
#include "vector_add_hal.hpp"

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  VectorAddHAL accelerator;

  SharedMemorySegment shm_h2a("shm_h2a", false);
  SharedMemorySegment shm_a2h("shm_a2h", true);

  logLine("[HAL] Started. Waiting for requests...\n");

  while (true) {
    // --- Receive Request (Reader Logic) ---
    auto* req_ptr = shm_h2a.base;

    // Signal Ready
    shm_h2a.setFlag(READY_BIT);
    while (!shm_h2a.isFlagSet(VALID_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    shm_h2a.clearFlag(READY_BIT);

    // Decode Packet
    int len_a = getFlit(req_ptr, 0);
    int len_b = getFlit(req_ptr, 1);

    std::vector<uint32_t> vec_a, vec_b;
    for (int i = 0; i < len_a; ++i) vec_a.push_back(getData(req_ptr, 0, i));
    for (int i = 0; i < len_b; ++i) vec_b.push_back(getData(req_ptr, 1, i));

    // Handshake ACK
    shm_h2a.setFlag(ACK_BIT);
    // Wait Host clear VALID
    while (shm_h2a.isFlagSet(VALID_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    shm_h2a.clearFlag(ACK_BIT);  // Clear ACK ourselves or wait Host?

    // --- Compute ---
    logLine("[HAL] Computing... Size: " + vec_a.size() + "\n");
    std::vector<uint32_t> vec_c = hal.compute(vec_a, vec_b);

    // --- Send Response (Writer Logic) ---
    auto* resp_ptr = shm_a2h.base;

    // Wait Host Ready
    while (!shm_a2h.isFlagSet(READY_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Serialize Result (Payload[0] = C)
    setPacketId(resp_ptr, getPacketId(req_ptr));  // Keep same ID
    setPayloadCount(resp_ptr, 1);
    setAddr(resp_resp, 0, 0);
    setFlit(resp_ptr, vec_c.size(), 0);
    for (size_t i = 0; i < vec_c.size(); ++i) setDataWord(resp_ptr, vec_c[i], 0, i);

    // Handshake
    shm_a2h.setFlag(VALID_BIT);
    while (!shm_a2h.isFlagSet(ACK_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    // Host (Reader) sets ACK, then waits for us (Writer) to clear ACK
    shm_a2h.clearFlag(ACK_BIT);
    shm_a2h.clearFlag(VALID_BIT);

    logLine("[HAL] Response Sent.\n");
  }

  return 0;
}