# Makefile
# ==============================================================================
#  1. PROJECT DIRECTORY LAYOUT & VARIABLES
#     Defines the core directory structure and include paths.
# ==============================================================================
MODULE      := vector_add
RTL_DIR     := rtl
CPP_DIR     := hal
OBJ_DIR     := obj_dir
SHM_DIR     := shm
RUNTIME_DIR := runtime

CPP_INCLUDES := -I$(abspath $(CPP_DIR)/include) -I$(abspath $(SHM_DIR))


# ==============================================================================
#  2. CODE FORMATTING CONFIGURATION
#     Defines tools, rules, and exclusions for C/C++ and Python code formatting.
# ==============================================================================
# --- Shared Exclusion Rules ---
EXCLUDE_DIRS  := build obj_dir aisrt.egg-info __pycache__ .venv
FIND_EXCLUDES := $(foreach dir,$(EXCLUDE_DIRS),-path "./$(dir)" -prune -o)

# --- C/C++ Formatter (Clang-Format) ---
C_FORMAT_EXTS   := c cpp cc h hpp hh
C_FORMATTER     := clang-format -i
C_FIND_INCLUDES := -type f \( $(foreach ext,$(C_FORMAT_EXTS),-name "*.$(ext)" -o) -false \)

# --- Python Formatter (Ruff) ---
PY_FORMAT_EXTS   := py
PY_FORMATTER     := python3 -m ruff format
PY_FIND_INCLUDES := -type f \( $(foreach ext,$(PY_FORMAT_EXTS),-name "*.$(ext)" -o) -false \)


# ==============================================================================
#  3. VERILATOR BUILD FLAGS
#     Compilation and simulation flags for the Verilator RTL backend.
# ==============================================================================
#  --cc     : Create C++ output
#  --exe    : Link to create executable
#  --build  : Build the model automatically
#  -j 0     : Use all available CPU cores for compilation
#  --trace  : Enable waveform tracing (VCD/FST)
#  -Wall    : Enable all warnings
# ==============================================================================
VFLAGS := --cc --exe --build -j 0 -Wall --trace \
          -CFLAGS "$(CPP_INCLUDES)" \
          --top-module $(MODULE)


# ==============================================================================
#  4. SOURCE FILES & DYNAMIC EXTENSIONS
#     Tracks all source dependencies and dynamically resolves Python extensions.
# ==============================================================================
# --- Hardware & HAL Sources ---
RTL_SRCS     := $(RTL_DIR)/$(MODULE).sv
HAL_CPP_SRCS := $(wildcard $(CPP_DIR)/src/*.cpp) $(wildcard $(SHM_DIR)/*.cpp)

# --- Python Runtime Sources ---
RUNTIME_SRCS := setup.py \
                $(wildcard runtime/backends/verilog/*.cpp) \
                $(wildcard runtime/backends/verilog/*.h) \
                $(wildcard runtime/backends/cpu/*.cpp) \
                $(wildcard runtime/backends/cpu/*.h) \
                $(wildcard shm/*.cpp) \
                $(wildcard shm/*.h)

# --- Dynamic Shared Object (.so) Resolution ---
PY_EXT_SUFFIX := $(shell python3 -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))")
RTL_SO        := aisrt_rtl$(PY_EXT_SUFFIX)
CPU_SO        := aisrt_cpu$(PY_EXT_SUFFIX)

.PHONY: all hal runtime format clean test

all: hal runtime

# ==========================================
# Target: Build HAL (Server)
# ==========================================
hal:
	@echo "--- [HAL] Building HAL Server (Verilator) ---"
	verilator $(VFLAGS) $(RTL_SRCS) $(HAL_CPP_SRCS)
	@echo "HAL Build Complete: $(OBJ_DIR)/V$(MODULE)"

# ==========================================
# Target: Build Python Runtime (Client)
# ==========================================
runtime: $(RTL_SO) $(CPU_SO)
$(RTL_SO) $(CPU_SO): $(RUNTIME_SRCS)
	@echo "--- [Runtime] Building Python PyBind11 Backends (RTL & CPU) ---"
	pip install -e . --no-build-isolation 
	@echo "--- [Runtime] Build Complete ---"

# ==========================================
# Integration Test: Run everything together
# ==========================================
TEST_TIMEOUT ?= 15s
test: all
	@echo "--- Starting Integration Test ---"
	@echo "1. Starting HAL Server in background..."
	@./$(OBJ_DIR)/V$(MODULE) & echo $$! > hal.pid
	@sleep 1
	
	@echo "2. Running Python Frontend Script (Timeout: $(TEST_TIMEOUT))..."
	@timeout $(TEST_TIMEOUT) python3 examples/test_vadd.py \
	|| (echo "Python Script Failed or Timed Out!" && kill `cat hal.pid` 2>/dev/null && rm -f hal.pid && exit 1)
	
	@echo "3. Shutting down HAL Server..."
	@kill `cat hal.pid` 2>/dev/null || true
	@rm -f hal.pid
	@echo "--- Integration Test Complete ---"

# ==========================================
# Utilities
# ==========================================
format:
	@echo "--- Formatting C/C++ Code ---"
	find . $(FIND_EXCLUDES) $(C_FIND_INCLUDES) -print0 | xargs -0 -r $(C_FORMATTER)
	@echo "--- Formatting Python Code ---"
	find . $(FIND_EXCLUDES) $(PY_FIND_INCLUDES) -print0 | xargs -0 -r $(PY_FORMATTER)
	@echo "--- Format Done ---"

clean:
	@echo "--- Cleaning Build Artifacts ---"
	rm -rf $(OBJ_DIR) dump.vcd build aisrt.egg-info
	find . -type d -name __pycache__ -exec rm -rf {} +
	find . -type f -name "*.so" -not -path "./.venv/*" -delete