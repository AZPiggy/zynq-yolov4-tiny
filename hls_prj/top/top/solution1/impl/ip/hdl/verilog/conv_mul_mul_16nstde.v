
`timescale 1 ns / 1 ps

  module conv_mul_mul_16nstde_DSP48_13(clk, rst, ce, a, b, p);
input clk;
input rst;
input ce;
input [16 - 1 : 0] a;
input [16 - 1 : 0] b;
output [18 - 1 : 0] p;

reg [18 - 1 : 0] p_reg; 


always @ (posedge clk) begin
    if (ce) begin
        p_reg <= $unsigned (a) * $unsigned (b);
    end
end

assign p = p_reg;

endmodule
`timescale 1 ns / 1 ps
module conv_mul_mul_16nstde(
    clk,
    reset,
    ce,
    din0,
    din1,
    dout);

parameter ID = 32'd1;
parameter NUM_STAGE = 32'd1;
parameter din0_WIDTH = 32'd1;
parameter din1_WIDTH = 32'd1;
parameter dout_WIDTH = 32'd1;
input clk;
input reset;
input ce;
input[din0_WIDTH - 1:0] din0;
input[din1_WIDTH - 1:0] din1;
output[dout_WIDTH - 1:0] dout;



conv_mul_mul_16nstde_DSP48_13 conv_mul_mul_16nstde_DSP48_13_U(
    .clk( clk ),
    .rst( reset ),
    .ce( ce ),
    .a( din0 ),
    .b( din1 ),
    .p( dout ));

endmodule

