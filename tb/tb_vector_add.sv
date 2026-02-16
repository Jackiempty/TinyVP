// tb/tb_vector_add.sv
`timescale 1ns/1ps

module tb_vector_add;

    // Parameters
    localparam DATA_WIDTH = 32;
    localparam VEC_LEN    = 4;

    // Signals
    logic clk;
    logic rst_n;
    logic in_valid;
    logic [DATA_WIDTH-1:0] vec_a [VEC_LEN-1:0];
    logic [DATA_WIDTH-1:0] vec_b [VEC_LEN-1:0];
    logic out_valid;
    logic [DATA_WIDTH-1:0] vec_c [VEC_LEN-1:0];

    // Instantiate DUT
    vector_add #(
        .DATA_WIDTH(DATA_WIDTH),
        .VEC_LEN(VEC_LEN)
    ) dut (
        .clk(clk),
        .rst_n(rst_n),
        .in_valid(in_valid),
        .vec_a(vec_a),
        .vec_b(vec_b),
        .out_valid(out_valid),
        .vec_c(vec_c)
    );

    // Clock Generation
    initial begin
        clk = 0;
        forever #5 clk = ~clk;
    end

    // Test Procedure
    initial begin
        $dumpfile("dump.vcd");
        $dumpvars(0, tb_vector_add);

        // 1. Initialize
        rst_n = 0;
        in_valid = 0;
        for(int i=0; i<VEC_LEN; i++) begin
            vec_a[i] = 0;
            vec_b[i] = 0;
        end
        
        // 2. Reset Release
        #20 rst_n = 1;

        // 3. Drive Inputs (T=30ns)
        @(negedge clk);
        in_valid = 1;
        for(int i=0; i<VEC_LEN; i++) begin
            vec_a[i] = i * 10;
            vec_b[i] = i + 1;
        end

        // 4. Wait one cycle, Stop Input AND Check Output (T=40ns)
        @(negedge clk);
        in_valid = 0;

        if (out_valid && vec_c[0] === 1 && vec_c[1] === 12) begin
            $display("--------------------------------");
            $display("[PASS] Vector Add Test Passed!");
            $display("vec_c[0] = %0d (Expected 1)", vec_c[0]);
            $display("vec_c[1] = %0d (Expected 12)", vec_c[1]);
            $display("--------------------------------");
        end else begin
            $display("--------------------------------");
            $display("[FAIL] Output mismatch or invalid!");
            $display("Time: %0t, out_valid: %b, vec_c[0]: %0d", $time, out_valid, vec_c[0]);
            $display("--------------------------------");
        end

        // End Simulation
        #20;
        $finish;
    end

endmodule
