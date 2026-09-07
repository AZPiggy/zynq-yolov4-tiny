#ifndef BASIC_BLOCK_H_INCLUDED
#define BASIC_BLOCK_H_INCLUDED
#include"basic_op.h"
#include<string>
#include<iostream>
using namespace std;

class BasicConv{
public:
    //閿熸枻鎷烽敓鏂ゆ嫹
    int h;
    int w;
    int s;
    int k;
    int p;
    int ch_in;
    int ch_out;
    //鏉冮敓鏂ゆ嫹
    data_t* weight;
    data_t* bias;
    //閿熸枻鎷烽敓鏂ゆ嫹
    BasicConv(int h,int w,int k,int s,int p,int ch_in,int ch_out){
        this->h=h;
        this->w=w;
        this->k=k;
        this->s=s;
        this->p=p;
        this->ch_in=ch_in;
        this->ch_out=ch_out;
        this->weight=new data_t[ch_out*ch_in*k*k];
        this->bias=new data_t[ch_out];
    }
    void forward(data_t* in,data_t* out){                                    //folded_conv+leakyrelu
        conv_leakyrelu(this->ch_in,this->ch_out,this->p,this->s,this->k,this->h,this->w,in,this->weight,this->bias,out,1);
    }
    void load_weight(string dir){
        //閿熸枻鎷峰彇閿熸枻鎷烽敓鏂ゆ嫹鏉冮敓鏂ゆ嫹
        string filename;
        filename=dir+"\\w.bin";
        read_params(filename,this->weight,this->ch_out*this->ch_in*this->k*this->k);
        filename=dir+"\\b.bin";
        read_params(filename,this->bias,this->ch_out);
    }
};

class Resblock_body{
public:
    int h;
    int w;
    int ch_in;
    int ch_out;
    BasicConv* conv1;
    BasicConv* conv2;
    BasicConv* conv3;
    BasicConv* conv4;
    Resblock_body(int h,int w,int ch_in,int ch_out){
        this->h=h;
        this->w=w;
        this->ch_in=ch_in;
        this->ch_out=ch_out;
        //閿熸枻鎷峰閿熸枻鎷稡asicConv
        this->conv1=new BasicConv(this->h,this->w,3,1,1,this->ch_in,this->ch_out);                   //
        this->conv2=new BasicConv(this->h,this->w,3,1,1,this->ch_out/2,this->ch_out/2);
        this->conv3=new BasicConv(this->h,this->w,3,1,1,this->ch_out/2,this->ch_out/2);
        this->conv4=new BasicConv(this->h,this->w,1,1,0,this->ch_out,this->ch_out);
    }
    void load_weight(string dir){
        string filename;
        //conv1
        filename=dir+"\\w1.bin";
        read_params(filename,this->conv1->weight,this->ch_in*this->ch_out*9);
        filename=dir+"\\b1.bin";
        read_params(filename,this->conv1->bias,this->ch_out);
        //conv2
        filename=dir+"\\w2.bin";
        read_params(filename,this->conv2->weight,this->ch_out/2*this->ch_out/2*9);
        filename=dir+"\\b2.bin";
        read_params(filename,this->conv2->bias,this->ch_out/2);
        //conv3
        filename=dir+"\\w3.bin";
        read_params(filename,this->conv3->weight,this->ch_out/2*this->ch_out/2*9);
        filename=dir+"\\b3.bin";
        read_params(filename,this->conv3->bias,this->ch_out/2);
        //conv4
        filename=dir+"\\w4.bin";
        read_params(filename,this->conv4->weight,this->ch_out*this->ch_out*1);
        filename=dir+"\\b4.bin";
        read_params(filename,this->conv4->bias,this->ch_out);
    }
    //
    void forward(data_t* in,data_t* feat,data_t* out){
    	  static data_t route[1500000];
    	  static data_t conv3_out[1500000];
          //
          conv1->forward(in,route);
          conv2->forward(route+this->h*this->w*this->ch_out/2,conv3_out+this->ch_out/2*this->h*this->w);
          conv3->forward(conv3_out+this->ch_out/2*this->h*this->w,conv3_out);
          //
          conv4->forward(conv3_out,feat);
          //maxpool(concat(route,feat))
          sampling(route,out,this->ch_out,this->h,1);
          sampling(feat,out+this->ch_out*this->h*this->w/4,this->ch_out,this->h,1);
    }
};

class CSPDarkNet{
public:
    BasicConv* basic_conv1;
    BasicConv* basic_conv2;
    BasicConv* basic_conv3;
    Resblock_body* resblock1;
    Resblock_body* resblock2;
    Resblock_body* resblock3;
    CSPDarkNet(){
        this->basic_conv1=new BasicConv(416,416,3,2,1,3,32);
        this->basic_conv2=new BasicConv(208,208,3,2,1,32,64);
        this->basic_conv3=new BasicConv(13,13,3,1,1,512,512);
        this->resblock1=new Resblock_body(104,104,64,64);
        this->resblock2=new Resblock_body(52,52,128,128);
        this->resblock3=new Resblock_body(26,26,256,256);
    }
    void load_weight(string dir){
        this->basic_conv1->load_weight(dir+"\\BasicConv1");
        this->basic_conv2->load_weight(dir+"\\BasicConv2");
        this->basic_conv3->load_weight(dir+"\\BasicConv3");
        this->resblock1->load_weight(dir+"\\ResBlock1");
        this->resblock2->load_weight(dir+"\\ResBlock2");
        this->resblock3->load_weight(dir+"\\ResBlock3");
    }
    void forward(data_t* in,data_t* feat1,data_t* feat2){
    	static data_t basic_conv1_out[32*208*208];
		static data_t basic_conv2_out[64*104*104];
		static data_t resblock1_out[128*52*52];
		static data_t resblock2_out[256*26*26];
		static data_t resblock3_out[512*13*13];
		static data_t tmp[64*104*104];
        //forward
        this->basic_conv1->forward(in,basic_conv1_out);
        //cout<<"basic conv1 end\n";
        this->basic_conv2->forward(basic_conv1_out,basic_conv2_out);
        //cout<<"basic conv2 end\n";
        this->resblock1->forward(basic_conv2_out,tmp,resblock1_out);
        //cout<<"resblock1 end\n";
        this->resblock2->forward(resblock1_out,tmp,resblock2_out);
        //cout<<"resblock2 end\n";
        this->resblock3->forward(resblock2_out,feat1,resblock3_out);
        //cout<<"resblock3 end\n";
        this->basic_conv3->forward(resblock3_out,feat2);
    }
};

#endif // BASIC_BLOCK_H_INCLUDED
