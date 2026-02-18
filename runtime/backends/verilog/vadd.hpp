// runtime/backends/verilog/vadd.hpp
#include <torch/torch.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "shm.hpp"

void                  addVectorToPacket(Packet& pkt, const std::vector<uint32_t>& vec, uint64_t fake_addr);
std::vector<uint32_t> extractVectorFromPacket(const Packet& pkt, int index);
void sendRequestAndVerify(SharedMemorySegment& shm_req, SharedMemorySegment& shm_resp, const std::vector<uint32_t>& a,
                          const std::vector<uint32_t>& b);
int  vadd(torch::Tensor a, torch::Tensor b);