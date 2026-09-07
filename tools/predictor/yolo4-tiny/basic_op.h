#ifndef BASIC_OP_H_INCLUDED
#define BASIC_OP_H_INCLUDED

#include<cmath>
#include<string>
#include<iostream>
using namespace std;

void conv(int ch_in,int ch_out,int pad,int stride,int k,int h,int w,float* in,float *weight,float *bias,float *out);

void maxpool(float* in,float* out,int ch,int h,int w);

void leakyrelu(float* in,float* out,int ch,int h,int w,float a);

void read_params(string filename,float* x,int length);

void upsample(float* in,float* out,int h,int w,int ch);
#endif // BASIC_OP_H_INCLUDED
