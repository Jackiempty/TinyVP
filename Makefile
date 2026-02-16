# Makefile

# variables
MODULE = tb_vector_add
RTL_DIR = ./rtl
TB_DIR = ./tb
OBJ_DIR = ./obj_dir

# Verilator Flags
# --binary: Automatically creates a C++ wrapper and compiles it into an executable (Verilator 5.0+)
# --trace: Enables waveform tracing
# --timing: Supports delays in SystemVerilog (e.g., #5)
# -j 0: Uses all available CPU cores for compilation
VFLAGS = --binary -j 0 -Wall --trace --timing

# Input Files
SRCS = $(TB_DIR)/$(MODULE).sv $(RTL_DIR)/vector_add.sv

.PHONY: all run clean wave

all: run

# 1. Verilate & Compile
compile:
	verilator $(VFLAGS) --top $(MODULE) $(SRCS)

# 2. Run Simulation
run: compile
	@echo "--- Running Simulation ---"
	$(OBJ_DIR)/V$(MODULE)

# 3. View Waveform (Optional, requires GTKWave)
wave:
	gtkwave dump.vcd

clean:
	rm -rf $(OBJ_DIR) dump.vcd