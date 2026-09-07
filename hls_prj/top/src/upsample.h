#include<ap_fixed.h>
#include<iostream>
#include<ap_shift_reg.h>
using namespace std;

typedef ap_fixed<16,7,AP_RND,AP_SAT> data_t;       //9 fractional bits
//typedef int data_t;

void upsample(data_t* in,data_t* out);
