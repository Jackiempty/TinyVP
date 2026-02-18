// runtime/backends/verilog/vadd.cpp
#include "vadd.hpp"

void addVectorToPacket(Packet& pkt, const std::vector<uint32_t>& vec, uint64_t fake_addr) {
  Payload pl{};
  pl.addr          = fake_addr;
  pl.read          = 0;  // Write operation
  pl.flit_word_num = static_cast<int>(vec.size());
  pkt.payload.push_back(pl);

  DataPayload dp;
  for (uint32_t val : vec) { dp.data.push_back(static_cast<int>(val)); }
  pkt.data_payload.push_back(dp);
}

// --- Helper: Extract Response Vector from Packet ---
std::vector<uint32_t> extractVectorFromPacket(const Packet& pkt, int index) {
  std::vector<uint32_t> result;
  if (index < pkt.data_payload.size()) {
    for (int val : pkt.data_payload[index].data) { result.push_back(static_cast<uint32_t>(val)); }
  }
  return result;
}

// --- Driver Logic (Client) ---
void sendRequestAndVerify(SharedMemorySegment& shm_req, SharedMemorySegment& shm_resp, const std::vector<uint32_t>& a,
                          const std::vector<uint32_t>& b) {
  Packet req_pkt{};
  // set ID randomly
  req_pkt.packet_id = 100;

  // put Vector A into Payload[0]
  addVectorToPacket(req_pkt, a, 0xAAAA);
  // put Vector B into Payload[1]
  addVectorToPacket(req_pkt, b, 0xBBBB);

  // send data with writerThread
  auto* ptr = shm_req.base;

  // Wait READY
  std::cout << "[Host] Waiting HAL Ready...\n";
  while (!shm_req.isFlagSet(READY_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // Serialize
  setPacketId(ptr, req_pkt.packet_id);
  setPayloadCount(ptr, req_pkt.payload.size());

  for (size_t i = 0; i < req_pkt.payload.size(); ++i) {
    setAddr(ptr, req_pkt.payload[i].addr, i);
    setReq(ptr, 0, i);
    setFlit(ptr, req_pkt.payload[i].flit_word_num, i);

    const auto& dp = req_pkt.data_payload[i];
    for (size_t j = 0; j < dp.data.size(); ++j) { setDataWord(ptr, dp.data[j], i, j); }
  }

  // Handshake
  shm_req.setFlag(VALID_BIT);
  std::cout << "[Host] Request Sent. Waiting ACK...\n";
  while (!shm_req.isFlagSet(ACK_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
  shm_req.clearFlag(VALID_BIT);
  shm_req.clearFlag(ACK_BIT);  // Host clearFlag ACK

  // wait for Response (Host becomes Reader)
  std::cout << "[Host] Waiting Response...\n";
  auto* resp_ptr = shm_resp.base;

  // Signal Ready
  shm_resp.setFlag(READY_BIT);
  while (!shm_resp.isFlagSet(VALID_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
  shm_resp.clearFlag(READY_BIT);

  // Read Data (Payload[0] is the result)
  Packet      resp_pkt{};
  int         count = getFlit(resp_ptr, 0);
  DataPayload resp_dp;
  for (int i = 0; i < count; ++i) resp_dp.data.push_back(getData(resp_ptr, 0, i));
  resp_pkt.data_payload.push_back(resp_dp);

  // Handshake
  shm_resp.setFlag(ACK_BIT);
  while (shm_resp.isFlagSet(ACK_BIT)) std::this_thread::sleep_for(std::chrono::milliseconds(1));  // Wait HAL clear

  // 4. 驗證結果
  std::vector<uint32_t> vec_c = extractVectorFromPacket(resp_pkt, 0);
  std::cout << "[Host] Result Received: ";
  for (auto v : vec_c) std::cout << v << " ";
  std::cout << "\n";
}

int vadd(torch::Tensor a, torch::Tensor b) {
  SharedMemorySegment shm_h2a("shm_h2a", true);
  SharedMemorySegment shm_a2h("shm_a2h", true);

  sendRequestAndVerify(shm_h2a, shm_a2h, a, b);
  return 0;
}
