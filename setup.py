# setup.py
import os
from setuptools import setup, find_packages
from torch.utils.cpp_extension import BuildExtension, CppExtension

project_root = os.path.dirname(os.path.abspath(__file__))

ext_modules = [
    CppExtension(
        name="aisrt.backends.aisrt_rtl",
        sources=[
            "csrc/device/binding.cpp",
            "csrc/device/vadd/vadd.cpp",
            "hal/hal.cpp",
            "hal/ipc/ipc.cpp",
            "common/shm/shm.cpp",
        ],
        include_dirs=[
            project_root,
        ],
        extra_compile_args=["-O3", "-std=c++17"],
    ),
    CppExtension(
        name="aisrt.backends.aisrt_cpu",
        sources=[
            "csrc/cpu/linear.cpp",
        ],
        include_dirs=[
            project_root,
        ],
        extra_compile_args=["-O3", "-std=c++17"],
    ),
]

setup(
    name="aisrt",
    version="0.1.2",
    description="AI System Runtime with Hardware Abstraction Layer",
    ext_modules=ext_modules,
    cmdclass={"build_ext": BuildExtension},
    package_dir={"": "python"},
    packages=find_packages(where="python"),
)
