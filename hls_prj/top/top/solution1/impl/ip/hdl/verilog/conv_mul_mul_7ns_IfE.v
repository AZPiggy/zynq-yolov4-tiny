
`timescale 1 ns / 1 ps

  module conv_mul_mul_7ns_IfE_DSP48_19(clk, rst, ce, a, b, p);
input clk;
input rst;
input ce;
input [7 - 1 : 0] a;
input signed [16 - 1 : 0] b;
output signed [23 - 1 : 0] p;

reg signed [23 - 1 : 0] p_reg; 


always @ (posedge clk) begin
    if (ce) begin
        p_reg <= $signed ({1'b0, a}) * $signed (b);
    end
end

assign p = p_reg;

endmodule
`timescale 1 ns / 1 ps
module conv_mul_mul_7ns_IfE(
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



conv_mul_mul_7ns_IfE_DSP48_19 conv_mul_mul_7ns_IfE_DSP48_19_U(
    .clk( clk ),
    .rst( reset ),
    .ce( ce ),
    .a( din0 ),
    .b( din1 ),
    .p( dout ));

endmodule

