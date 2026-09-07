#ifndef BASIC_BLOCK_H_INCLUDED
#define BASIC_BLOCK_H_INCLUDED
#include"basic_op.h"
#include<string>
#include<iostream>
using namespace std;

class BasicConv{
public:
    //layer geometry
    int h;
    int w;
    int s;
    int k;
    int p;
    int ch_in;
    int ch_out;
    //parameters
    float* weight;
    float* bias;
    float alpha;                //leakyrelu
    //constructor
    BasicConv(int h,int w,int k,int s,int p,int ch_in,int ch_out){
        this->h=h;
        this->w=w;
        this->k=k;
        this->s=s;
        this->p=p;
        this->ch_in=ch_in;
        this->ch_out=ch_out;
        this->weight=new float[ch_out*ch_in*k*k];
        this->bias=new float[ch_out];
        alpha=0.1;
    }
    void init(int h,int w,int k,int s,int p,int ch_in,int ch_out){
        this->h=h;
        this->w=w;
        this->k=k;
        this->s=s;
        this->p=p;
        this->ch_in=ch_in;
        this->ch_out=ch_out;
        this->weight=new float[ch_out*ch_in*k*k];
        this->bias=new float[ch_out];
        this->alpha=0.1;
    }
    void forward(float* in,float* out){                                    //folded_conv+leakyrelu
        int max_length=(this->ch_out)*(this->h)*(this->w);
        float* conv_out=new float[max_length];
        conv(this->ch_in,this->ch_out,this->p,this->s,this->k,this->h,this->w,in,this->weight,this->bias,conv_out);
        if(this->s==2)
            leakyrelu(conv_out,out,this->ch_out,this->h/2,this->w/2,this->alpha);
        else
            leakyrelu(conv_out,out,this->ch_out,this->h,this->w,this->alpha);
        delete [] conv_out;        //dont forget!!
    }
    void load_weight(string dir){
        //read this layer's weights
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
        //build the BasicConv sub-layers
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
    void concat(float* a,float* b,float* out,int h,int w,int ch_out){                //ch_out is the channel count after concatenation, out=concat([a,b],axis=1)
            for(int i=0;i<h*w*ch_out/2;i++){
                  out[i]=a[i];
            }
            for(int i=0;i<h*w*ch_out/2;i++){
                  out[i+h*w*ch_out/2]=b[i];
            }
    }
    void forward(float* in,float* feat,float* out){
          float *route=new float[2*this->ch_out*this->h*this->w];                 //concat(true_route,feat)
          float *conv3_out=new float[this->ch_out*this->h*this->w];               //first half of conv3_out is conv3's output, second half is route1
          float *pool_in=new float[this->ch_out*2*this->h*this->w];               //concat(route,feat)
          //
          conv1->forward(in,route);
          conv2->forward(route+this->h*this->w*this->ch_out/2,conv3_out+this->ch_out/2*this->h*this->w);
          conv3->forward(conv3_out+this->ch_out/2*this->h*this->w,conv3_out);
          //concat
          //this->concat(conv3_out,route1,conv4_in,this->h,this->w,this->ch_out);
          conv4->forward(conv3_out,feat);
          //concat
          this->concat(route,feat,pool_in,this->h,this->w,this->ch_out*2);
          //pool
          maxpool(pool_in,out,this->ch_out*2,this->h,this->w);
          //feat=route+this->ch_out*this->h*this->w;
          delete [] route;
          delete [] conv3_out;
          delete [] pool_in;
    }
};

//void CSPDarkNet(float* in,float* out1,float* out2);
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
    void forward(float* in,float* feat1,float* feat2){
        float* basic_conv1_out=new float[32*208*208];                                           //3x416x416-->32x208x208
        float* basic_conv2_out=new float[64*104*104];                                           //32x208x208-->64x104x104
        float* resblock1_out=new float[128*52*52];                                                 //64x104x104-->128x52x52
        float* resblock2_out=new float[256*26*26];                                                 //128x52x52-->256x26x26
        float* resblock3_out=new float[512*13*13];                                                 //256x26x26-->512x13x13
        float* tmp=new float[64*104*104];                                                                                  //max(64x104x104,128x52x52,256x26x26)=64x104x104
        //forward
        this->basic_conv1->forward(in,basic_conv1_out);
        this->basic_conv2->forward(basic_conv1_out,basic_conv2_out);
        this->resblock1->forward(basic_conv2_out,tmp,resblock1_out);
        this->resblock2->forward(resblock1_out,tmp,resblock2_out);
        this->resblock3->forward(resblock2_out,feat1,resblock3_out);
        this->basic_conv3->forward(resblock3_out,feat2);
        //delete
        delete [] basic_conv1_out;
        delete [] basic_conv2_out;
        delete [] resblock1_out;
        delete [] resblock2_out;
        delete [] resblock3_out;
        delete [] tmp;
    }
};

#endif // BASIC_BLOCK_H_INCLUDED
