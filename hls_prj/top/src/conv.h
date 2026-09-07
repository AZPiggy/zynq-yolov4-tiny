#include"pwconv.h"
#include"std_conv.h"
#include"maxpool.h"

void conv(data_t* in1,data_t* in2,data_t* in3,data_t* in4,data_t* w1,data_t* w2,data_t* w3,
data_t* w4,data_t* b,data_t* out1,data_t* out2,data_t* out3,data_t* out4,int ch_in,int ch_out,
int fsize,int stride,int kernel,int act);
