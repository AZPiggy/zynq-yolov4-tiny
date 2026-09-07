// ==============================================================
// Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC v2019.2 (64-bit)
// Copyright 1986-2019 Xilinx, Inc. All Rights Reserved.
// ==============================================================
`timescale 1ns/1ps
module conv_CTRL_s_axi
#(parameter
    C_S_AXI_ADDR_WIDTH = 8,
    C_S_AXI_DATA_WIDTH = 32
)(
    input  wire                          ACLK,
    input  wire                          ARESET,
    input  wire                          ACLK_EN,
    input  wire [C_S_AXI_ADDR_WIDTH-1:0] AWADDR,
    input  wire                          AWVALID,
    output wire                          AWREADY,
    input  wire [C_S_AXI_DATA_WIDTH-1:0] WDATA,
    input  wire [C_S_AXI_DATA_WIDTH/8-1:0] WSTRB,
    input  wire                          WVALID,
    output wire                          WREADY,
    output wire [1:0]                    BRESP,
    output wire                          BVALID,
    input  wire                          BREADY,
    input  wire [C_S_AXI_ADDR_WIDTH-1:0] ARADDR,
    input  wire                          ARVALID,
    output wire                          ARREADY,
    output wire [C_S_AXI_DATA_WIDTH-1:0] RDATA,
    output wire [1:0]                    RRESP,
    output wire                          RVALID,
    input  wire                          RREADY,
    output wire                          interrupt,
    output wire                          ap_start,
    input  wire                          ap_done,
    input  wire                          ap_ready,
    input  wire                          ap_idle,
    output wire [31:0]                   in1_V,
    output wire [31:0]                   in2_V,
    output wire [31:0]                   in3_V,
    output wire [31:0]                   in4_V,
    output wire [31:0]                   w1_V,
    output wire [31:0]                   w2_V,
    output wire [31:0]                   w3_V,
    output wire [31:0]                   w4_V,
    output wire [31:0]                   b_V,
    output wire [31:0]                   out1_V,
    output wire [31:0]                   out2_V,
    output wire [31:0]                   out3_V,
    output wire [31:0]                   out4_V,
    output wire [31:0]                   ch_in,
    output wire [31:0]                   ch_out,
    output wire [31:0]                   fsize,
    output wire [31:0]                   stride,
    output wire [31:0]                   kernel,
    output wire [31:0]                   act
);
//------------------------Address Info-------------------
// 0x00 : Control signals
//        bit 0  - ap_start (Read/Write/COH)
//        bit 1  - ap_done (Read/COR)
//        bit 2  - ap_idle (Read)
//        bit 3  - ap_ready (Read)
//        bit 7  - auto_restart (Read/Write)
//        others - reserved
// 0x04 : Global Interrupt Enable Register
//        bit 0  - Global Interrupt Enable (Read/Write)
//        others - reserved
// 0x08 : IP Interrupt Enable Register (Read/Write)
//        bit 0  - Channel 0 (ap_done)
//        bit 1  - Channel 1 (ap_ready)
//        others - reserved
// 0x0c : IP Interrupt Status Register (Read/TOW)
//        bit 0  - Channel 0 (ap_done)
//        bit 1  - Channel 1 (ap_ready)
//        others - reserved
// 0x10 : Data signal of in1_V
//        bit 31~0 - in1_V[31:0] (Read/Write)
// 0x14 : reserved
// 0x18 : Data signal of in2_V
//        bit 31~0 - in2_V[31:0] (Read/Write)
// 0x1c : reserved
// 0x20 : Data signal of in3_V
//        bit 31~0 - in3_V[31:0] (Read/Write)
// 0x24 : reserved
// 0x28 : Data signal of in4_V
//        bit 31~0 - in4_V[31:0] (Read/Write)
// 0x2c : reserved
// 0x30 : Data signal of w1_V
//        bit 31~0 - w1_V[31:0] (Read/Write)
// 0x34 : reserved
// 0x38 : Data signal of w2_V
//        bit 31~0 - w2_V[31:0] (Read/Write)
// 0x3c : reserved
// 0x40 : Data signal of w3_V
//        bit 31~0 - w3_V[31:0] (Read/Write)
// 0x44 : reserved
// 0x48 : Data signal of w4_V
//        bit 31~0 - w4_V[31:0] (Read/Write)
// 0x4c : reserved
// 0x50 : Data signal of b_V
//        bit 31~0 - b_V[31:0] (Read/Write)
// 0x54 : reserved
// 0x58 : Data signal of out1_V
//        bit 31~0 - out1_V[31:0] (Read/Write)
// 0x5c : reserved
// 0x60 : Data signal of out2_V
//        bit 31~0 - out2_V[31:0] (Read/Write)
// 0x64 : reserved
// 0x68 : Data signal of out3_V
//        bit 31~0 - out3_V[31:0] (Read/Write)
// 0x6c : reserved
// 0x70 : Data signal of out4_V
//        bit 31~0 - out4_V[31:0] (Read/Write)
// 0x74 : reserved
// 0x78 : Data signal of ch_in
//        bit 31~0 - ch_in[31:0] (Read/Write)
// 0x7c : reserved
// 0x80 : Data signal of ch_out
//        bit 31~0 - ch_out[31:0] (Read/Write)
// 0x84 : reserved
// 0x88 : Data signal of fsize
//        bit 31~0 - fsize[31:0] (Read/Write)
// 0x8c : reserved
// 0x90 : Data signal of stride
//        bit 31~0 - stride[31:0] (Read/Write)
// 0x94 : reserved
// 0x98 : Data signal of kernel
//        bit 31~0 - kernel[31:0] (Read/Write)
// 0x9c : reserved
// 0xa0 : Data signal of act
//        bit 31~0 - act[31:0] (Read/Write)
// 0xa4 : reserved
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

//------------------------Parameter----------------------
localparam
    ADDR_AP_CTRL       = 8'h00,
    ADDR_GIE           = 8'h04,
    ADDR_IER           = 8'h08,
    ADDR_ISR           = 8'h0c,
    ADDR_IN1_V_DATA_0  = 8'h10,
    ADDR_IN1_V_CTRL    = 8'h14,
    ADDR_IN2_V_DATA_0  = 8'h18,
    ADDR_IN2_V_CTRL    = 8'h1c,
    ADDR_IN3_V_DATA_0  = 8'h20,
    ADDR_IN3_V_CTRL    = 8'h24,
    ADDR_IN4_V_DATA_0  = 8'h28,
    ADDR_IN4_V_CTRL    = 8'h2c,
    ADDR_W1_V_DATA_0   = 8'h30,
    ADDR_W1_V_CTRL     = 8'h34,
    ADDR_W2_V_DATA_0   = 8'h38,
    ADDR_W2_V_CTRL     = 8'h3c,
    ADDR_W3_V_DATA_0   = 8'h40,
    ADDR_W3_V_CTRL     = 8'h44,
    ADDR_W4_V_DATA_0   = 8'h48,
    ADDR_W4_V_CTRL     = 8'h4c,
    ADDR_B_V_DATA_0    = 8'h50,
    ADDR_B_V_CTRL      = 8'h54,
    ADDR_OUT1_V_DATA_0 = 8'h58,
    ADDR_OUT1_V_CTRL   = 8'h5c,
    ADDR_OUT2_V_DATA_0 = 8'h60,
    ADDR_OUT2_V_CTRL   = 8'h64,
    ADDR_OUT3_V_DATA_0 = 8'h68,
    ADDR_OUT3_V_CTRL   = 8'h6c,
    ADDR_OUT4_V_DATA_0 = 8'h70,
    ADDR_OUT4_V_CTRL   = 8'h74,
    ADDR_CH_IN_DATA_0  = 8'h78,
    ADDR_CH_IN_CTRL    = 8'h7c,
    ADDR_CH_OUT_DATA_0 = 8'h80,
    ADDR_CH_OUT_CTRL   = 8'h84,
    ADDR_FSIZE_DATA_0  = 8'h88,
    ADDR_FSIZE_CTRL    = 8'h8c,
    ADDR_STRIDE_DATA_0 = 8'h90,
    ADDR_STRIDE_CTRL   = 8'h94,
    ADDR_KERNEL_DATA_0 = 8'h98,
    ADDR_KERNEL_CTRL   = 8'h9c,
    ADDR_ACT_DATA_0    = 8'ha0,
    ADDR_ACT_CTRL      = 8'ha4,
    WRIDLE             = 2'd0,
    WRDATA             = 2'd1,
    WRRESP             = 2'd2,
    WRRESET            = 2'd3,
    RDIDLE             = 2'd0,
    RDDATA             = 2'd1,
    RDRESET            = 2'd2,
    ADDR_BITS         = 8;

//------------------------Local signal-------------------
    reg  [1:0]                    wstate = WRRESET;
    reg  [1:0]                    wnext;
    reg  [ADDR_BITS-1:0]          waddr;
    wire [31:0]                   wmask;
    wire                          aw_hs;
    wire                          w_hs;
    reg  [1:0]                    rstate = RDRESET;
    reg  [1:0]                    rnext;
    reg  [31:0]                   rdata;
    wire                          ar_hs;
    wire [ADDR_BITS-1:0]          raddr;
    // internal registers
    reg                           int_ap_idle;
    reg                           int_ap_ready;
    reg                           int_ap_done = 1'b0;
    reg                           int_ap_start = 1'b0;
    reg                           int_auto_restart = 1'b0;
    reg                           int_gie = 1'b0;
    reg  [1:0]                    int_ier = 2'b0;
    reg  [1:0]                    int_isr = 2'b0;
    reg  [31:0]                   int_in1_V = 'b0;
    reg  [31:0]                   int_in2_V = 'b0;
    reg  [31:0]                   int_in3_V = 'b0;
    reg  [31:0]                   int_in4_V = 'b0;
    reg  [31:0]                   int_w1_V = 'b0;
    reg  [31:0]                   int_w2_V = 'b0;
    reg  [31:0]                   int_w3_V = 'b0;
    reg  [31:0]                   int_w4_V = 'b0;
    reg  [31:0]                   int_b_V = 'b0;
    reg  [31:0]                   int_out1_V = 'b0;
    reg  [31:0]                   int_out2_V = 'b0;
    reg  [31:0]                   int_out3_V = 'b0;
    reg  [31:0]                   int_out4_V = 'b0;
    reg  [31:0]                   int_ch_in = 'b0;
    reg  [31:0]                   int_ch_out = 'b0;
    reg  [31:0]                   int_fsize = 'b0;
    reg  [31:0]                   int_stride = 'b0;
    reg  [31:0]                   int_kernel = 'b0;
    reg  [31:0]                   int_act = 'b0;

//------------------------Instantiation------------------

//------------------------AXI write fsm------------------
assign AWREADY = (wstate == WRIDLE);
assign WREADY  = (wstate == WRDATA);
assign BRESP   = 2'b00;  // OKAY
assign BVALID  = (wstate == WRRESP);
assign wmask   = { {8{WSTRB[3]}}, {8{WSTRB[2]}}, {8{WSTRB[1]}}, {8{WSTRB[0]}} };
assign aw_hs   = AWVALID & AWREADY;
assign w_hs    = WVALID & WREADY;

// wstate
always @(posedge ACLK) begin
    if (ARESET)
        wstate <= WRRESET;
    else if (ACLK_EN)
        wstate <= wnext;
end

// wnext
always @(*) begin
    case (wstate)
        WRIDLE:
            if (AWVALID)
                wnext = WRDATA;
            else
                wnext = WRIDLE;
        WRDATA:
            if (WVALID)
                wnext = WRRESP;
            else
                wnext = WRDATA;
        WRRESP:
            if (BREADY)
                wnext = WRIDLE;
            else
                wnext = WRRESP;
        default:
            wnext = WRIDLE;
    endcase
end

// waddr
always @(posedge ACLK) begin
    if (ACLK_EN) begin
        if (aw_hs)
            waddr <= AWADDR[ADDR_BITS-1:0];
    end
end

//------------------------AXI read fsm-------------------
assign ARREADY = (rstate == RDIDLE);
assign RDATA   = rdata;
assign RRESP   = 2'b00;  // OKAY
assign RVALID  = (rstate == RDDATA);
assign ar_hs   = ARVALID & ARREADY;
assign raddr   = ARADDR[ADDR_BITS-1:0];

// rstate
always @(posedge ACLK) begin
    if (ARESET)
        rstate <= RDRESET;
    else if (ACLK_EN)
        rstate <= rnext;
end

// rnext
always @(*) begin
    case (rstate)
        RDIDLE:
            if (ARVALID)
                rnext = RDDATA;
            else
                rnext = RDIDLE;
        RDDATA:
            if (RREADY & RVALID)
                rnext = RDIDLE;
            else
                rnext = RDDATA;
        default:
            rnext = RDIDLE;
    endcase
end

// rdata
always @(posedge ACLK) begin
    if (ACLK_EN) begin
        if (ar_hs) begin
            rdata <= 1'b0;
            case (raddr)
                ADDR_AP_CTRL: begin
                    rdata[0] <= int_ap_start;
                    rdata[1] <= int_ap_done;
                    rdata[2] <= int_ap_idle;
                    rdata[3] <= int_ap_ready;
                    rdata[7] <= int_auto_restart;
                end
                ADDR_GIE: begin
                    rdata <= int_gie;
                end
                ADDR_IER: begin
                    rdata <= int_ier;
                end
                ADDR_ISR: begin
                    rdata <= int_isr;
                end
                ADDR_IN1_V_DATA_0: begin
                    rdata <= int_in1_V[31:0];
                end
                ADDR_IN2_V_DATA_0: begin
                    rdata <= int_in2_V[31:0];
                end
                ADDR_IN3_V_DATA_0: begin
                    rdata <= int_in3_V[31:0];
                end
                ADDR_IN4_V_DATA_0: begin
                    rdata <= int_in4_V[31:0];
                end
                ADDR_W1_V_DATA_0: begin
                    rdata <= int_w1_V[31:0];
                end
                ADDR_W2_V_DATA_0: begin
                    rdata <= int_w2_V[31:0];
                end
                ADDR_W3_V_DATA_0: begin
                    rdata <= int_w3_V[31:0];
                end
                ADDR_W4_V_DATA_0: begin
                    rdata <= int_w4_V[31:0];
                end
                ADDR_B_V_DATA_0: begin
                    rdata <= int_b_V[31:0];
                end
                ADDR_OUT1_V_DATA_0: begin
                    rdata <= int_out1_V[31:0];
                end
                ADDR_OUT2_V_DATA_0: begin
                    rdata <= int_out2_V[31:0];
                end
                ADDR_OUT3_V_DATA_0: begin
                    rdata <= int_out3_V[31:0];
                end
                ADDR_OUT4_V_DATA_0: begin
                    rdata <= int_out4_V[31:0];
                end
                ADDR_CH_IN_DATA_0: begin
                    rdata <= int_ch_in[31:0];
                end
                ADDR_CH_OUT_DATA_0: begin
                    rdata <= int_ch_out[31:0];
                end
                ADDR_FSIZE_DATA_0: begin
                    rdata <= int_fsize[31:0];
                end
                ADDR_STRIDE_DATA_0: begin
                    rdata <= int_stride[31:0];
                end
                ADDR_KERNEL_DATA_0: begin
                    rdata <= int_kernel[31:0];
                end
                ADDR_ACT_DATA_0: begin
                    rdata <= int_act[31:0];
                end
            endcase
        end
    end
end


//------------------------Register logic-----------------
assign interrupt = int_gie & (|int_isr);
assign ap_start  = int_ap_start;
assign in1_V     = int_in1_V;
assign in2_V     = int_in2_V;
assign in3_V     = int_in3_V;
assign in4_V     = int_in4_V;
assign w1_V      = int_w1_V;
assign w2_V      = int_w2_V;
assign w3_V      = int_w3_V;
assign w4_V      = int_w4_V;
assign b_V       = int_b_V;
assign out1_V    = int_out1_V;
assign out2_V    = int_out2_V;
assign out3_V    = int_out3_V;
assign out4_V    = int_out4_V;
assign ch_in     = int_ch_in;
assign ch_out    = int_ch_out;
assign fsize     = int_fsize;
assign stride    = int_stride;
assign kernel    = int_kernel;
assign act       = int_act;
// int_ap_start
always @(posedge ACLK) begin
    if (ARESET)
        int_ap_start <= 1'b0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_AP_CTRL && WSTRB[0] && WDATA[0])
            int_ap_start <= 1'b1;
        else if (ap_ready)
            int_ap_start <= int_auto_restart; // clear on handshake/auto restart
    end
end

// int_ap_done
always @(posedge ACLK) begin
    if (ARESET)
        int_ap_done <= 1'b0;
    else if (ACLK_EN) begin
        if (ap_done)
            int_ap_done <= 1'b1;
        else if (ar_hs && raddr == ADDR_AP_CTRL)
            int_ap_done <= 1'b0; // clear on read
    end
end

// int_ap_idle
always @(posedge ACLK) begin
    if (ARESET)
        int_ap_idle <= 1'b0;
    else if (ACLK_EN) begin
            int_ap_idle <= ap_idle;
    end
end

// int_ap_ready
always @(posedge ACLK) begin
    if (ARESET)
        int_ap_ready <= 1'b0;
    else if (ACLK_EN) begin
            int_ap_ready <= ap_ready;
    end
end

// int_auto_restart
always @(posedge ACLK) begin
    if (ARESET)
        int_auto_restart <= 1'b0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_AP_CTRL && WSTRB[0])
            int_auto_restart <=  WDATA[7];
    end
end

// int_gie
always @(posedge ACLK) begin
    if (ARESET)
        int_gie <= 1'b0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_GIE && WSTRB[0])
            int_gie <= WDATA[0];
    end
end

// int_ier
always @(posedge ACLK) begin
    if (ARESET)
        int_ier <= 1'b0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_IER && WSTRB[0])
            int_ier <= WDATA[1:0];
    end
end

// int_isr[0]
always @(posedge ACLK) begin
    if (ARESET)
        int_isr[0] <= 1'b0;
    else if (ACLK_EN) begin
        if (int_ier[0] & ap_done)
            int_isr[0] <= 1'b1;
        else if (w_hs && waddr == ADDR_ISR && WSTRB[0])
            int_isr[0] <= int_isr[0] ^ WDATA[0]; // toggle on write
    end
end

// int_isr[1]
always @(posedge ACLK) begin
    if (ARESET)
        int_isr[1] <= 1'b0;
    else if (ACLK_EN) begin
        if (int_ier[1] & ap_ready)
            int_isr[1] <= 1'b1;
        else if (w_hs && waddr == ADDR_ISR && WSTRB[0])
            int_isr[1] <= int_isr[1] ^ WDATA[1]; // toggle on write
    end
end

// int_in1_V[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_in1_V[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_IN1_V_DATA_0)
            int_in1_V[31:0] <= (WDATA[31:0] & wmask) | (int_in1_V[31:0] & ~wmask);
    end
end

// int_in2_V[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_in2_V[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_IN2_V_DATA_0)
            int_in2_V[31:0] <= (WDATA[31:0] & wmask) | (int_in2_V[31:0] & ~wmask);
    end
end

// int_in3_V[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_in3_V[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_IN3_V_DATA_0)
            int_in3_V[31:0] <= (WDATA[31:0] & wmask) | (int_in3_V[31:0] & ~wmask);
    end
end

// int_in4_V[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_in4_V[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_IN4_V_DATA_0)
            int_in4_V[31:0] <= (WDATA[31:0] & wmask) | (int_in4_V[31:0] & ~wmask);
    end
end

// int_w1_V[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_w1_V[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_W1_V_DATA_0)
            int_w1_V[31:0] <= (WDATA[31:0] & wmask) | (int_w1_V[31:0] & ~wmask);
    end
end

// int_w2_V[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_w2_V[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_W2_V_DATA_0)
            int_w2_V[31:0] <= (WDATA[31:0] & wmask) | (int_w2_V[31:0] & ~wmask);
    end
end

// int_w3_V[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_w3_V[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_W3_V_DATA_0)
            int_w3_V[31:0] <= (WDATA[31:0] & wmask) | (int_w3_V[31:0] & ~wmask);
    end
end

// int_w4_V[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_w4_V[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_W4_V_DATA_0)
            int_w4_V[31:0] <= (WDATA[31:0] & wmask) | (int_w4_V[31:0] & ~wmask);
    end
end

// int_b_V[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_b_V[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_B_V_DATA_0)
            int_b_V[31:0] <= (WDATA[31:0] & wmask) | (int_b_V[31:0] & ~wmask);
    end
end

// int_out1_V[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_out1_V[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_OUT1_V_DATA_0)
            int_out1_V[31:0] <= (WDATA[31:0] & wmask) | (int_out1_V[31:0] & ~wmask);
    end
end

// int_out2_V[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_out2_V[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_OUT2_V_DATA_0)
            int_out2_V[31:0] <= (WDATA[31:0] & wmask) | (int_out2_V[31:0] & ~wmask);
    end
end

// int_out3_V[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_out3_V[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_OUT3_V_DATA_0)
            int_out3_V[31:0] <= (WDATA[31:0] & wmask) | (int_out3_V[31:0] & ~wmask);
    end
end

// int_out4_V[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_out4_V[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_OUT4_V_DATA_0)
            int_out4_V[31:0] <= (WDATA[31:0] & wmask) | (int_out4_V[31:0] & ~wmask);
    end
end

// int_ch_in[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_ch_in[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_CH_IN_DATA_0)
            int_ch_in[31:0] <= (WDATA[31:0] & wmask) | (int_ch_in[31:0] & ~wmask);
    end
end

// int_ch_out[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_ch_out[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_CH_OUT_DATA_0)
            int_ch_out[31:0] <= (WDATA[31:0] & wmask) | (int_ch_out[31:0] & ~wmask);
    end
end

// int_fsize[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_fsize[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_FSIZE_DATA_0)
            int_fsize[31:0] <= (WDATA[31:0] & wmask) | (int_fsize[31:0] & ~wmask);
    end
end

// int_stride[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_stride[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_STRIDE_DATA_0)
            int_stride[31:0] <= (WDATA[31:0] & wmask) | (int_stride[31:0] & ~wmask);
    end
end

// int_kernel[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_kernel[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_KERNEL_DATA_0)
            int_kernel[31:0] <= (WDATA[31:0] & wmask) | (int_kernel[31:0] & ~wmask);
    end
end

// int_act[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_act[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_ACT_DATA_0)
            int_act[31:0] <= (WDATA[31:0] & wmask) | (int_act[31:0] & ~wmask);
    end
end


//------------------------Memory logic-------------------

endmodule
