# Tiny-VP: Hardware/Software Co-Simulation Framework

Tiny-VP is a high-performance hardware/software co-design and simulation framework. It bridges Python-based deep learning runtimes (like PyTorch) and SystemVerilog RTL designs (simulated via Verilator) using a **high-throughput, low-latency Shared Memory (SHM) Inter-Process Communication (IPC) mechanism**. 

This architecture allows for seamless dispatching of tensor computations from a Python frontend directly to a C++ Hardware Abstraction Layer (HAL) and RTL simulator, completely bypassing the overhead of heavy network sockets, OS pipes, or file I/O.

## Prerequisites

Before building the project, ensure your system meets the following requirements:

* **Operating System**: Linux (Ubuntu recommended)
* **C++ Compiler**: `g++` or `clang++` with **C++17** support
* **Verilator**: Version **5.0+** (Required for `--build` and `--exe` auto-generation)
* **Python**: Python 3.8+ (Python 3.11 recommended)
* **Make**: GNU Make

## Python Dependencies

It is highly recommended to use a Python virtual environment (`.venv`). 

Install the required Python packages. **Note:** If you are running on a headless server or a machine without an NVIDIA GPU, please install the CPU-only version of PyTorch to avoid dynamic linking errors (`libtorch_global_deps.so` not found) during the C++ extension build.

```bash
# 1. Create and activate a virtual environment
python3 -m venv .venv
source .venv/bin/activate

# 2. Install PyTorch (CPU version recommended for pure RTL simulation)
pip install torch --index-url https://download.pytorch.org/whl/cpu

# 3. Install build dependencies
pip install pybind11 setuptools
```

## Getting Started & Build Instructions

This project uses `make` to manage the build process for both the Hardware Abstraction Layer (HAL) Server via Verilator and the Python Frontend via PyBind11.

### Quick Start: Build & Test
The easiest way to compile everything and verify that the host and hardware simulator are communicating correctly is to run the integration test:

```bash
make test

```

**What this does:**

1. Compiles the RTL SystemVerilog files into a C++ executable HAL server.
2. Compiles the C++ runtime backends (`RTL` and `CPU`) into Python `.so` extensions.
3. Starts the HAL server in the background.
4. Executes the Python test script (`examples/test_vadd.py`).
5. Gracefully shuts down the HAL server upon completion.

### Detailed Build Commands

If you need to build or manage individual components, use the following `make` targets:

| Command | Description |
| --- | --- |
| `make all` | Builds both the HAL server and the Python runtime extensions. |
| `make hal` | Compiles only the Verilator RTL backend (`obj_dir/Vvector_add`). |
| `make runtime` | Builds only the Python PyBind11 bindings (`aisrt_rtl.so` & `aisrt_cpu.so`). |
| `make clean` | Removes all compiled binaries, `.so` files, and build cache directories. |
| `make format` | Runs `clang-format` on C/C++ files and `ruff` on Python scripts. |

### Debugging IPC (Shared Memory)

Since the Python frontend and Verilator backend communicate asynchronously via Shared Memory (SHM), deadlocks can occur during development.

To trace the detailed `VALID`/`READY`/`ACK` handshaking between the Host and the Accelerator, run the test with the debug flag enabled:

```bash
make test DEBUG_SHM=1

```

This will output verbose symmetric logs from both the Host and the HAL, allowing you to easily pinpoint where the communication stalled.
