// rtl/vector_add.sv
module vector_add #(
    parameter DATA_WIDTH = 32,
    parameter VEC_LEN    = 4
)(
    input  logic clk,
    input  logic rst_n,

    // --- Configuration Interface (CSR) ---
    input  logic [31:0]             cfg_length,
    input  logic                    cfg_start,

    // Input Interface
    input  logic                    in_valid,
    input  logic signed [DATA_WIDTH-1:0]   vec_a [VEC_LEN-1:0],
    input  logic signed [DATA_WIDTH-1:0]   vec_b [VEC_LEN-1:0],

    // Output Interface
    output logic                    out_valid,
    output logic signed [DATA_WIDTH-1:0]   vec_c [VEC_LEN-1:0],

    // --- Interrupt / Status ---
    output logic                    irq_done,
    output logic                    busy
);
    logic [31:0] counter;
    logic        is_running;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            out_valid  <= 1'b0;
            irq_done   <= 1'b0;
            busy       <= 1'b0;
            counter    <= 0;
            is_running <= 0;
            for (int i = 0; i < VEC_LEN; i++) vec_c[i] <= '0;
        end else begin
            if (cfg_start && !busy) begin
                busy       <= 1'b1;
                is_running <= 1'b1;
                counter    <= 0;
                irq_done   <= 1'b0;
                out_valid  <= 1'b0;
            end 
            else if (is_running) begin
                if (in_valid) begin
                    out_valid <= 1'b1;
                    for (int i = 0; i < VEC_LEN; i++) begin
                        vec_c[i] <= vec_a[i] + vec_b[i];
                    end
                    
                    counter <= counter + 1;

                    if (counter == cfg_length - 1) begin
                        is_running <= 1'b0; 
                    end
                end else begin
                    out_valid <= 1'b0;
                end
            end 
            else if (busy && !is_running) begin
                busy      <= 1'b0;
                irq_done  <= 1'b1;
                out_valid <= 1'b0;
            end 
            else begin
                irq_done  <= 1'b0;
                out_valid <= 1'b0;
            end
        end
    end

endmodule
