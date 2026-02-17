// cpp/main.cpp
#include <iostream>
#include <vector>

#include "shm.hpp"
#include "vector_add_hal.hpp"

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);

  VectorAddHAL accelerator;

  std::cout << "--- [C++] Starting Simulation ---" << std::endl;
  std::vector<uint32_t> host_a = {10, 20, 30, 40};
  std::vector<uint32_t> host_b = {1, 2, 3, 4};
  std::vector<uint32_t> host_c = accelerator.compute(host_a, host_b);

  bool pass = true;
  for (int i = 0; i < 4; i++) {
    uint32_t expected = host_a[i] + host_b[i];
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