# setup.py
import os
from setuptools import setup, find_packages
from torch.utils.cpp_extension import BuildExtension, CppExtension

project_root = os.path.dirname(os.path.abspath(__file__))

ext_modules = [
    CppExtension(
        name="aisrt.backends.aisrt_rtl",
        sources=[
            "csrc/verilog/binding.cpp",
            "csrc/verilog/vadd.cpp",
            "common/shm/shm.cpp",
        ],
        include_dirs=[
            os.path.join(project_root, "common", "shm"),
        ],
        extra_compile_args=["-O3", "-std=c++17"],
    ),
    CppExtension(
        name="aisrt.backends.aisrt_cpu",
        sources=[
            "csrc/cpu/linear.cpp",
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
    package_dir={"": "python"},
    packages=find_packages(where="python"),
)
