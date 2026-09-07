// ==============================================================
// Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC v2019.2 (64-bit)
// Copyright 1986-2019 Xilinx, Inc. All Rights Reserved.
// ==============================================================
// CTRL
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

#define XCONV_CTRL_ADDR_AP_CTRL     0x00
#define XCONV_CTRL_ADDR_GIE         0x04
#define XCONV_CTRL_ADDR_IER         0x08
#define XCONV_CTRL_ADDR_ISR         0x0c
#define XCONV_CTRL_ADDR_IN1_V_DATA  0x10
#define XCONV_CTRL_BITS_IN1_V_DATA  32
#define XCONV_CTRL_ADDR_IN2_V_DATA  0x18
#define XCONV_CTRL_BITS_IN2_V_DATA  32
#define XCONV_CTRL_ADDR_IN3_V_DATA  0x20
#define XCONV_CTRL_BITS_IN3_V_DATA  32
#define XCONV_CTRL_ADDR_IN4_V_DATA  0x28
#define XCONV_CTRL_BITS_IN4_V_DATA  32
#define XCONV_CTRL_ADDR_W1_V_DATA   0x30
#define XCONV_CTRL_BITS_W1_V_DATA   32
#define XCONV_CTRL_ADDR_W2_V_DATA   0x38
#define XCONV_CTRL_BITS_W2_V_DATA   32
#define XCONV_CTRL_ADDR_W3_V_DATA   0x40
#define XCONV_CTRL_BITS_W3_V_DATA   32
#define XCONV_CTRL_ADDR_W4_V_DATA   0x48
#define XCONV_CTRL_BITS_W4_V_DATA   32
#define XCONV_CTRL_ADDR_B_V_DATA    0x50
#define XCONV_CTRL_BITS_B_V_DATA    32
#define XCONV_CTRL_ADDR_OUT1_V_DATA 0x58
#define XCONV_CTRL_BITS_OUT1_V_DATA 32
#define XCONV_CTRL_ADDR_OUT2_V_DATA 0x60
#define XCONV_CTRL_BITS_OUT2_V_DATA 32
#define XCONV_CTRL_ADDR_OUT3_V_DATA 0x68
#define XCONV_CTRL_BITS_OUT3_V_DATA 32
#define XCONV_CTRL_ADDR_OUT4_V_DATA 0x70
#define XCONV_CTRL_BITS_OUT4_V_DATA 32
#define XCONV_CTRL_ADDR_CH_IN_DATA  0x78
#define XCONV_CTRL_BITS_CH_IN_DATA  32
#define XCONV_CTRL_ADDR_CH_OUT_DATA 0x80
#define XCONV_CTRL_BITS_CH_OUT_DATA 32
#define XCONV_CTRL_ADDR_FSIZE_DATA  0x88
#define XCONV_CTRL_BITS_FSIZE_DATA  32
#define XCONV_CTRL_ADDR_STRIDE_DATA 0x90
#define XCONV_CTRL_BITS_STRIDE_DATA 32
#define XCONV_CTRL_ADDR_KERNEL_DATA 0x98
#define XCONV_CTRL_BITS_KERNEL_DATA 32
#define XCONV_CTRL_ADDR_ACT_DATA    0xa0
#define XCONV_CTRL_BITS_ACT_DATA    32

