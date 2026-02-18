PYTHON ?= python3
CXX := g++
# NVCC := nvcc

CXXFLAGS := -std=c++17 -shared -fPIC -Wall -Wextra
# NVCCFLAGS := -std=c++17 -Xcompiler -fPIC
CPPFLAGS := $(shell $(PYTHON) -m pybind11 --includes)
# LDFLAGS := -L/usr/local/cuda/lib64 -lcudart

LIB := aisrt_rtl$(shell $(PYTHON) -m pybind11 --extension-suffix)
PREFIX ?= $(shell $(PYTHON) -c "import site; print(site.getsitepackages()[0])")

OBJS := vadd.o binding.o

PYTHON := $(shell which python3)
TORCH_PATH := $(shell $(PYTHON) -c "import torch; print(torch.__path__[0])")
PYTHON_INC := $(shell $(PYTHON) -c "from sysconfig import get_paths; print(get_paths()['include'])")

INCLUDES := -I$(TORCH_PATH)/include -I$(TORCH_PATH)/include/torch/csrc/api/include
LIBS := -L$(TORCH_PATH)/lib

.PHONY: all release debug clean install

all: release

release: CXXFLAGS += -O3 -DNDEBUG
release: CXXFLAGS += $(INCLUDES)
release: LDFLAGS += $(LIBS) -ltorch -ltorch_cpu -ltorch_cuda -ltorch_python -lc10 -Wl,-rpath,$(TORCH_PATH)/lib
# release: NVCCFLAGS += $(INCLUDES)
# release: NVCCFLAGS += -I$(PYTHON_INC)
release: $(LIB)

debug: CXXFLAGS += -g -O0 -DDEBUG
debug: $(LIB)

install: release
	cp $(LIB) $(PREFIX)/$(LIB)

$(LIB): $(OBJS)
	$(CXX) $(CPPFLAGS) -shared -fPIC -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

# %.o: %.cu
# 	$(NVCC) $(NVCCFLAGS) -c -o $@ $<

clean:
	@$(RM) -v $(LIB) $(PREFIX)/$(LIB) $(OBJS)
