#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <torch/torch.h>

#include <iostream>

#include "vadd.hpp"

namespace py = pybind11;

torch::Tensor vadd_wrapper(torch::Tensor in1, torch::Tensor in2) {
  int           len = in1.numel();
  torch::Tensor out = vadd(in1, in2);  // call RTL kernel
  return out;
}

void help() { std::cout << "Usage: output = nn.vadd(input1, input2)" << std::endl; }

PYBIND11_MODULE(aisrt_rtl, m) {
  m.def("vadd", &vadd_wrapper, "Custom vector addition for RTL backend");
  m.def("help", &help, "Self-implmeneted RTL kernels");
}
