// cpp/vector_add_hal.hpp
#include <vector>
#include <iostream>
#include <verilated.h>
#include "Vvector_add.h"

class VectorAddHAL {
private:
    Vvector_add* dut;
    vluint64_t main_time = 0;

    void tick() {
        dut->clk = 0; 
        dut->eval();
        
        dut->clk = 1; 
        dut->eval();
        
        main_time++;
    }

public:
    VectorAddHAL() {
        dut = new Vvector_add;
        reset();
    }

    ~VectorAddHAL() {
        dut->final();
        delete dut;
    }

    void reset() {
        dut->rst_n = 0;
        dut->in_valid = 0;
        for(int i=0; i<5; i++) tick(); 
        dut->rst_n = 1;
        tick();
    }

    std::vector<uint32_t> compute(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
        if (a.size() != 4 || b.size() != 4) {
            std::cerr << "Error: Input size must be 4" << std::endl;
            return {};
        }

        dut->in_valid = 1;
        for (int i = 0; i < 4; i++) {
            dut->vec_a[i] = a[i];
            dut->vec_b[i] = b[i];
        }

        tick();

        std::vector<uint32_t> result(4);
        
        if (dut->out_valid) {
            for (int i = 0; i < 4; i++) {
                result[i] = dut->vec_c[i];
            }
        } else {
            std::cerr << "Warning: Output not valid at Cycle " << main_time << std::endl;
        }
        dut->in_valid = 0;
        tick();

        return result;
    }
};