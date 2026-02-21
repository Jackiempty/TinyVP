# setup.py
import os
from setuptools import setup, Extension
from torch.utils.cpp_extension import BuildExtension, CppExtension

project_root = os.path.dirname(os.path.abspath(__file__))

ext_modules = [
    CppExtension(
        name="aisrt_rtl",
        sources=[
            "runtime/backends/verilog/binding.cpp",
            "runtime/backends/verilog/vadd.cpp",
            "shm/shm.cpp",
        ],
        include_dirs=[
            os.path.join(project_root, "shm"),
        ],
        extra_compile_args=["-O3", "-std=c++17"],
    ),
    CppExtension(
        name="aisrt_cpu",
        sources=[
            "runtime/backends/cpu/linear.cpp",
        ],
        extra_compile_args=["-O3", "-std=c++17"],
    ),
]

setup(
    name="aisrt",
    version="0.1.0",
    description="AI System Runtime with Verilator RTL Backend",
    ext_modules=ext_modules,
    cmdclass={"build_ext": BuildExtension},
    packages=["runtime", "runtime.nn"],
)
