PYTHON ?= python3
CXX := g++

CXXFLAGS := -std=c++11 -shared -fPIC -Wall -Wextra
CPPFLAGS := $(shell $(PYTHON) -m pybind11 --includes)

LIB := aisrt_cpu$(shell $(PYTHON) -m pybind11 --extension-suffix)
PREFIX ?= $(shell $(PYTHON) -c "import site; print(site.getsitepackages()[0])")

OBJS := linear.o

.PHONY: all release debug clean install

all: release

release: CXXFLAGS += -O3 -DNDEBUG
release: $(LIB)

debug: CXXFLAGS += -g -O0 -DDEBUG
debug: $(LIB)

install: release
	cp $(LIB) $(PREFIX)/$(LIB)

$(LIB): $(OBJS)
	$(CXX) $(CPPFLAGS) -shared -fPIC -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	@$(RM) -v $(LIB) $(PREFIX)/$(LIB) $(OBJS)
