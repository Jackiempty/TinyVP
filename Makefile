# Makefile
# ==============================================================================
#  1. PROJECT DIRECTORY LAYOUT & VARIABLES
#     Defines the core directory structure and include paths.
# ==============================================================================
MODULE      := vector_add
RTL_DIR     := rtl/operators
CPP_DIR     := hal
SHM_DIR     := common/shm
CSRC_DIR    := csrc
PY_PKG_DIR  := python/aisrt

# --- Build Directory ---
BUILD_DIR   := build
OBJ_DIR     := $(BUILD_DIR)/obj_dir

# --- Debug Toggles ---
# DEBUG is the master switch. You can also override them individually:
# e.g., make test DEBUG=1 (Enables both)
# e.g., make test DEBUG_HOST=1 (Enables only Host logs)
DEBUG      ?= 0
DEBUG_HOST ?= $(DEBUG)
DEBUG_HAL  ?= $(DEBUG)

# --- Root-Relative Includes ---
PROJECT_ROOT := $(abspath .)
CPP_INCLUDES := -I$(PROJECT_ROOT) $(DEBUG_FLAGS)


# ==============================================================================
#  2. CODE FORMATTING CONFIGURATION
#     Defines tools, rules, and exclusions for C/C++ and Python code formatting.
# ==============================================================================
# --- Shared Exclusion Rules ---
EXCLUDE_DIRS  := build python/aisrt.egg-info __pycache__ .venv
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
          --Mdir $(OBJ_DIR) \
          --top-module $(MODULE)


# ==============================================================================
#  4. SOURCE FILES & DYNAMIC EXTENSIONS
#     Tracks all source dependencies and dynamically resolves Python extensions.
# ==============================================================================
# --- Hardware & HAL Sources ---
RTL_SRCS     := $(RTL_DIR)/$(MODULE).sv
HAL_CPP_SRCS := $(wildcard $(CPP_DIR)/*.cpp) $(wildcard $(CPP_DIR)/operators/*.cpp) $(wildcard $(SHM_DIR)/*.cpp)

# --- Python Runtime Sources ---
RUNTIME_SRCS := setup.py \
                $(wildcard $(CSRC_DIR)/verilog/*.cpp) \
                $(wildcard $(CSRC_DIR)/verilog/*.hpp) \
                $(wildcard $(CSRC_DIR)/cpu/*.cpp) \
                $(wildcard $(SHM_DIR)/*.cpp) \
                $(wildcard $(SHM_DIR)/*.hpp)

# --- Dynamic Shared Object (.so) Resolution ---
PY_EXT_SUFFIX := $(shell python3 -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))")
RTL_SO        := $(PY_PKG_DIR)/backends/aisrt_rtl$(PY_EXT_SUFFIX)
CPU_SO        := $(PY_PKG_DIR)/backends/aisrt_cpu$(PY_EXT_SUFFIX)

.PHONY: all hal runtime format clean test compdb

all: hal runtime

# ==========================================
# Target: Build HAL (Server)
# ==========================================
hal:
	@echo "--- [HAL] Building HAL Server (Verilator) ---"
	@mkdir -p $(BUILD_DIR)
	verilator $(VFLAGS) $(RTL_SRCS) $(HAL_CPP_SRCS)
	@echo "HAL Build Complete: $(OBJ_DIR)/V$(MODULE)"

# ==========================================
# Target: Build Python Runtime (Client)
# ==========================================
runtime: $(RTL_SO) $(CPU_SO)
$(RTL_SO) $(CPU_SO): $(RUNTIME_SRCS)
	@echo "--- [Runtime] Building Python PyBind11 Backends (RTL & CPU) ---"
	CFLAGS="$(DEBUG_FLAGS)" CXXFLAGS="$(DEBUG_FLAGS)" pip install -e . --no-build-isolation
	@echo "--- [Runtime] Build Complete ---"

# ==========================================
# Integration Test
# ==========================================
TEST_TIMEOUT ?= 15s
test: all
	@echo "--- Starting Integration Test ---"
	@mkdir -p $(BUILD_DIR)
	@echo "1. Starting HAL Server in background..."
	@./$(OBJ_DIR)/V$(MODULE) & echo $$! > $(BUILD_DIR)/hal.pid
	@sleep 1
	
	@echo "2. Running Python Frontend Script (Timeout: $(TEST_TIMEOUT))..."
	@timeout $(TEST_TIMEOUT) python3 examples/test_vadd.py \
	|| (echo "Python Script Failed or Timed Out!" && kill `cat $(BUILD_DIR)/hal.pid` 2>/dev/null && rm -f $(BUILD_DIR)/hal.pid && exit 1)
	
	@echo "3. Shutting down HAL Server..."
	@kill `cat $(BUILD_DIR)/hal.pid` 2>/dev/null || true
	@rm -f $(BUILD_DIR)/hal.pid
	@echo "--- Integration Test Complete ---"

# ==========================================
# Utilities
# ==========================================
compdb: clean
	@echo "--- Generating compile_commands.json using bear ---"
	bear -- $(MAKE) all
	@echo "--- compile_commands.json generated successfully ---"

format:
	@echo "--- Formatting C/C++ Code ---"
	find . $(FIND_EXCLUDES) $(C_FIND_INCLUDES) -print0 | xargs -0 -r $(C_FORMATTER)
	@echo "--- Formatting Python Code ---"
	find . $(FIND_EXCLUDES) $(PY_FIND_INCLUDES) -print0 | xargs -0 -r $(PY_FORMATTER)
	@echo "--- Format Done ---"

clean:
	@echo "--- Cleaning Build Artifacts ---"
	rm -rf $(BUILD_DIR) dump.vcd python/aisrt.egg-info compile_commands.json
	find . -type d -name __pycache__ -exec rm -rf {} +
	find . -type f -name "*.so" -not -path "./.venv/*" -delete
	rm -f /dev/shm/aisrt_shm