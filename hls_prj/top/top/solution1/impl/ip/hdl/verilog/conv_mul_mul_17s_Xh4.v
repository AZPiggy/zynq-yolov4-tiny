
`timescale 1 ns / 1 ps

  module conv_mul_mul_17s_Xh4_DSP48_22(clk, rst, ce, a, b, p);
input clk;
input rst;
input ce;
input signed [17 - 1 : 0] a;
input [16 - 1 : 0] b;
output signed [32 - 1 : 0] p;

reg signed [32 - 1 : 0] p_reg; 


always @ (posedge clk) begin
    if (ce) begin
        p_reg <= $signed (a) * $signed ({1'b0, b});
    end
end

assign p = p_reg;

endmodule
`timescale 1 ns / 1 ps
module conv_mul_mul_17s_Xh4(
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



conv_mul_mul_17s_Xh4_DSP48_22 conv_mul_mul_17s_Xh4_DSP48_22_U(
    .clk( clk ),
    .rst( reset ),
    .ce( ce ),
    .a( din0 ),
    .b( din1 ),
    .p( dout ));

endmodule

