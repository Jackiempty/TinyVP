// csrc/cpu/linear.cpp
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include <iostream>

namespace py = pybind11;

void _linear(float* output, float* inputs, float* weights, float* bias, int in_features, int out_features) {
  for (int i = 0; i < out_features; i++) {
    output[i] = 0.0f;
    for (int j = 0; j < in_features; j++) { output[i] += inputs[j] * weights[i * in_features + j]; }
    output[i] += bias[i];
  }
}

void linear(py::array_t<float> output, py::array_t<float> inputs, py::array_t<float> weights, py::array_t<float> bias) {
  auto   info = weights.request();
  float* w    = static_cast<float*>(info.ptr);
  float* i    = static_cast<float*>(inputs.request().ptr);
  float* o    = static_cast<float*>(output.request().ptr);
  float* b    = static_cast<float*>(bias.request().ptr);

  int out_features = info.shape[0];
  int in_features  = info.shape[1];

  std::printf("[CPU_INFO] linear(out_features=%d, in_features=%d) for CPU is called.\n", out_features, in_features);
  _linear(o, i, w, b, in_features, out_features);
}

void help() { std::cout << "Usage: functional.linear(output, inputs, weight, bias)" << std::endl; }

PYBIND11_MODULE(aisrt_cpu, m) {
  m.def("linear", &linear, "Custom Linear layer for CPU backend");
  m.def("help", &help, "Self-implmeneted linear kernel");
}
