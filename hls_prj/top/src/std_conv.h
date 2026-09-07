#include<ap_fixed.h>
#include<ap_int.h>
#include<stdio.h>
#include<string.h>
#define Tr 13
#define Tc 13
#define Tn 4
#define Tm 32
#define K 3
#define P 1
#define MAX_LEN 512
#define TRin 27            //(Tr-1)*S+K=27
#define TCin 27

typedef ap_fixed<16,7,AP_RND,AP_SAT> data_t;
//typedef float data_t;


void std_conv(data_t* in1,data_t* in2,data_t* in3,data_t* in4,data_t* w1,data_t* w2,data_t* w3,
data_t* w4,data_t* bias,data_t* out1,data_t* out2,data_t* out3,data_t* out4,unsigned short ch_in,
unsigned short ch_out,unsigned short fm_size,unsigned short stride,unsigned short act);

