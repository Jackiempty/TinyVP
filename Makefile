# Makefile
# variables
MODULE = vector_add
RTL_DIR = ./rtl
CPP_DIR = ./cpp
OBJ_DIR = ./obj_dir

# Verilator Flags
# --binary: Automatically creates a C++ wrapper and compiles it into an executable (Verilator 5.0+)
# --trace: Enables waveform tracing
# --timing: Supports delays in SystemVerilog (e.g., #5)
# -j 0: Uses all available CPU cores for compilation
VFLAGS = --cc --exe --build -j 0 -Wall --trace \
         -I$(CPP_DIR) \
         --top-module $(MODULE)

# Source Files
RTL_SRCS = $(RTL_DIR)/$(MODULE).sv
CPP_SRCS = $(CPP_DIR)/main.cpp

.PHONY: all run clean

all: run

run:
	@echo "--- Verilating & Building ---"
	verilator $(VFLAGS) $(RTL_SRCS) $(CPP_SRCS)
	@echo "--- Running C++ Simulation ---"
	$(OBJ_DIR)/V$(MODULE)

clean:
	rm -rf $(OBJ_DIR) dump.vcd