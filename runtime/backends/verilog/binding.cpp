#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <torch/torch.h>

#include <iostream>

#include "mmul.cuh"
#include "mtrans.cuh"
#include "vadd.cuh"

namespace py = pybind11;

static inline void print_tensor_info(const torch::Tensor& t, const std::string& name);

torch::Tensor vadd_wrapper(torch::Tensor in1, torch::Tensor in2) {
  int           len = in1.numel();
  torch::Tensor out = vadd(in1, in2);  // call CUDA kernel
  return out;
}

torch::Tensor qadd_wrapper(torch::Tensor in1, torch::Tensor in2, float scale, int32_t zero_point) {
  int           len = in1.numel();
  torch::Tensor out = qadd(in1, in2, scale, zero_point);  // call CUDA kernel
  return out;
}

torch::Tensor mmul_wrapper(torch::Tensor in1, torch::Tensor in2) {
  int           len = in1.size(0) * in2.size(1);
  torch::Tensor out = mmul(in1, in2);  // call CUDA kernel
  return out;
}

torch::Tensor mtrans_wrapper(torch::Tensor in) {
  int           len = in.numel();
  torch::Tensor out = mtrans(in);  // call CUDA kernel
  return out;
}

void help() { std::cout << "Usage: functional.vadd(output, input1, input2)" << std::endl; }

PYBIND11_MODULE(aisrt_cuda, m) {
  m.def("vadd", &vadd_wrapper, "Custom vector addition for CUDA backend");
  m.def("qadd", &qadd_wrapper, "Custom quantized vector addition for CUDA backend");
  m.def("mmul", &mmul_wrapper, "Matrix multiplication for CUDA backend");
  m.def("mtrans", &mtrans_wrapper, "Matrix transpose for CUDA backend");
  m.def("help", &help, "Self-implmeneted CUDA kernels");
}

static inline void print_tensor_info(const torch::Tensor& t, const std::string& name = "") {
  if (!name.empty()) {
    std::cout << "[Tensor Info] " << name << std::endl;
  } else {
    std::cout << "[Tensor Info]" << std::endl;
  }

  // basic components
  std::cout << "  dtype: " << t.dtype() << std::endl;
  std::cout << "  device: " << t.device() << std::endl;
  std::cout << "  is_cuda: " << (t.is_cuda() ? "True" : "False") << std::endl;
  std::cout << "  is_contiguous: " << (t.is_contiguous() ? "True" : "False") << std::endl;

  // shape
  std::cout << "  shape: [";
  for (size_t i = 0; i < t.dim(); ++i) {
    std::cout << t.size(i);
    if (i != t.dim() - 1) std::cout << ", ";
  }
  std::cout << "]" << std::endl;

  // element numbers
  std::cout << "  numel: " << t.numel() << std::endl;

  // print some value randomly
  try {
    torch::Tensor cpu_t = t.to(torch::kCPU);
    auto          flat  = cpu_t.flatten();

    int print_len = std::min<int>(flat.numel(), 10);
    std::cout << "  values[0:" << print_len << "]: ";
    std::cout << flat.slice(0, 0, print_len);
  } catch (const c10::Error& e) { std::cerr << "  (Cannot print values: " << e.msg() << ")" << std::endl; }
  std::cout << std::endl;
}
