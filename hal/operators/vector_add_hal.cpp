// hal/operators/vector_add_hal.cpp
#include "vector_add_hal.hpp"

VectorAddHAL::VectorAddHAL() {
  dut = new Vvector_add;
  reset();
}

VectorAddHAL::~VectorAddHAL() {
  dut->final();
  delete dut;
}

void VectorAddHAL::tick() {
  dut->clk = 0;
  dut->eval();
  dut->clk = 1;
  dut->eval();
  main_time++;
}

void VectorAddHAL::reset() {
  dut->rst_n     = 0;
  dut->cfg_start = 0;
  dut->in_valid  = 0;
  for (int i = 0; i < 5; i++) tick();
  dut->rst_n = 1;
  tick();
}

void VectorAddHAL::compute(const int32_t* a, const int32_t* b, int32_t* result, size_t total_size) {
  if (total_size == 0 || a == nullptr || b == nullptr || result == nullptr) return;
  uint32_t batch_count = (total_size + VEC_LEN - 1) / VEC_LEN;

  dut->cfg_length = batch_count;
  dut->cfg_start  = 1;
  tick();
  dut->cfg_start = 0;

  size_t sent_idx       = 0;
  size_t received_count = 0;

  while (!dut->irq_done) {
    if (sent_idx < total_size) {
      dut->in_valid = 1;
      for (int i = 0; i < VEC_LEN; i++) {
        if (sent_idx + i < total_size) {
          dut->vec_a[i] = a[sent_idx + i];
          dut->vec_b[i] = b[sent_idx + i];
        } else {
          dut->vec_a[i] = 0;
          dut->vec_b[i] = 0;
        }
      }
      sent_idx += VEC_LEN;
    } else {
      dut->in_valid = 0;
    }

    tick();

    if (dut->out_valid) {
      for (int i = 0; i < VEC_LEN; i++) {
        if (received_count < total_size) { result[received_count++] = dut->vec_c[i]; }
      }
    }

    if (main_time > 1000000) {
      std::cerr << "Timeout! Hardware stuck." << std::endl;
      break;
    }
  }

  if (dut->out_valid) {
    for (int i = 0; i < VEC_LEN; i++) {
      if (received_count < total_size) { result[received_count++] = dut->vec_c[i]; }
    }
  }

  tick();
}