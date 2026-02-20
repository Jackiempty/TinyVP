#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <torch/torch.h>

#include <iostream>

#include "vadd.hpp"

namespace py = pybind11;

/**
 * @brief Wrapper for vector addition targeting the custom RTL backend.
 *
 * This function serves as the PyTorch C++ extension entry point. It converts
 * PyTorch tensors into standard C++ vectors, dispatches the workload to the
 * hardware abstraction layer (HAL), and reconstructs the result back into a tensor.
 *
 * @param in1 First input tensor (must be contiguous float32).
 * @param in2 Second input tensor (must be contiguous float32).
 * @return torch::Tensor The result of the hardware-accelerated vector addition.
 */
torch::Tensor vadd_wrapper(torch::Tensor in1, torch::Tensor in2) {
  // 1. Validate input tensor memory layout and data type
  TORCH_CHECK(in1.is_contiguous() && in2.is_contiguous(), "Inputs must be contiguous");
  TORCH_CHECK(in1.scalar_type() == torch::kInt32, "in1 must be Int32");
  TORCH_CHECK(in2.scalar_type() == torch::kInt32, "in2 must be Int32");
  TORCH_CHECK(in1.sizes() == in2.sizes(), "Input shapes must match");

  // 2. Extract raw data pointers and determine the total number of elements
  int32_t* in1_ptr = in1.data_ptr<int32_t>();
  int32_t* in2_ptr = in2.data_ptr<int32_t>();
  uint32_t size    = in1.numel();

  // 3. Construct the output tensor
  // Allocate uninitialized memory for the output tensor
  auto          options = torch::TensorOptions().dtype(torch::kInt32).device(in1.device());
  torch::Tensor output  = torch::empty({size}, options);
  int32_t*      out_ptr = output.data_ptr<int32_t>();

  // 4. Dispatch the computation to the hardware abstraction layer
  vadd(in1_ptr, in2_ptr, out_ptr, size);

  return output;
}

void help() { std::cout << "Usage: output = nn.vadd(input1, input2)" << std::endl; }

PYBIND11_MODULE(aisrt_rtl, m) {
  m.def("vadd", &vadd_wrapper, "Custom vector addition for RTL backend");
  m.def("help", &help, "Self-implmeneted RTL kernels");
}
