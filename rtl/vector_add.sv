// rtl/vector_add.sv
module vector_add #(
    parameter DATA_WIDTH = 32,
    parameter VEC_LEN    = 4
)(
    input  logic clk,
    input  logic rst_n,
    
    // Input Interface
    input  logic                    in_valid,
    input  logic [DATA_WIDTH-1:0]   vec_a [VEC_LEN-1:0],
    input  logic [DATA_WIDTH-1:0]   vec_b [VEC_LEN-1:0],

    // Output Interface
    output logic                    out_valid,
    output logic [DATA_WIDTH-1:0]   vec_c [VEC_LEN-1:0]
);

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            out_valid <= 1'b0;
            for (int i = 0; i < VEC_LEN; i++) begin
                vec_c[i] <= '0;
            end
        end else begin
            out_valid <= in_valid;
            if (in_valid) begin
                for (int i = 0; i < VEC_LEN; i++) begin
                    vec_c[i] <= vec_a[i] + vec_b[i];
                end
            end
        end
    end

endmodule
