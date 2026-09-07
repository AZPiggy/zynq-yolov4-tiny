#ifndef YOLO4_TINY_H_INCLUDED
#define YOLO4_TINY_H_INCLUDED
#include"basic_block.h"

class Yolo4_Tiny{
public:
    CSPDarkNet* backbone;
    //conv_forP5
    BasicConv* conv_forP5_conv;
    //yolo_headP4
    BasicConv* yolo_headP4_basic_conv1;
    float* yolo_headP4_w2;
    float* yolo_headP4_b2;
    //yolo_headP5
    BasicConv* yolo_headP5_basic_conv1;
    float* yolo_headP5_w2;
    float* yolo_headP5_b2;
    int out_ch = 255;
    //upsample
    BasicConv* upsample_conv;
    Yolo4_Tiny(){           //constructor
        //this->out_ch=(4+1+class_num)*3
        this->backbone=new CSPDarkNet();
        //conv_forP5
        this->conv_forP5_conv=new BasicConv(13,13,1,1,0,512,256);         //K=1
        //yolo_headP4
        this->yolo_headP4_basic_conv1=new BasicConv(26,26,3,1,1,384,256);
        this->yolo_headP4_w2=new float[this->out_ch*256*1*1];                              //K=1
        this->yolo_headP4_b2=new float[this->out_ch];
        //yolo_headP5
        this->yolo_headP5_basic_conv1=new BasicConv(13,13,3,1,1,256,512);
        this->yolo_headP5_w2=new float[this->out_ch*512*1*1];                              //K=1
        this->yolo_headP5_b2=new float[this->out_ch];
        //upsample
        this->upsample_conv=new BasicConv(13,13,1,1,0,256,128);           //K=1
    }
    void load_weight(string dir){
        this->backbone->load_weight(dir);
        //conv_forP5
        this->conv_forP5_conv->load_weight(dir+"\\conv_forP5");
        //yolo_headP4
        read_params(dir+"\\yolo_headP4\\w1.bin",this->yolo_headP4_basic_conv1->weight,256*384*3*3);
        read_params(dir+"\\yolo_headP4\\b1.bin",this->yolo_headP4_basic_conv1->bias,256);
        read_params(dir+"\\yolo_headP4\\w2.bin",this->yolo_headP4_w2,this->out_ch*256*1*1);
        read_params(dir+"\\yolo_headP4\\b2.bin",this->yolo_headP4_b2,this->out_ch);
        //yolo_headP5
        read_params(dir+"\\yolo_headP5\\w1.bin",this->yolo_headP5_basic_conv1->weight,512*256*9);
        read_params(dir+"\\yolo_headP5\\b1.bin",this->yolo_headP5_basic_conv1->bias,512);
        read_params(dir+"\\yolo_headP5\\w2.bin",this->yolo_headP5_w2,this->out_ch*512*1*1);
        read_params(dir+"\\yolo_headP5\\b2.bin",this->yolo_headP5_b2,this->out_ch);
        //upsample
        this->upsample_conv->load_weight(dir+"\\upsample");
    }
    void concat(float* a,float* b,float* out){
          //concat([a,b],dim=1)-->out
          for(int i=0;i<128*26*26;i++){
                out[i]=a[i];
          }
          for(int i=0;i<256*26*26;i++){
                out[i+128*26*26]=b[i];
          }
    }
    void forward(float* in,float* out0,float* out1){
        float* feat1=new float[256*26*26];
        float* feat2=new float[512*13*13];
        float* P5=new float[256*13*13];
        float* out0_tmp=new float[512*13*13];
        float* P5_Upsample=new float[128*13*13];
        float* P4=new float[384*26*26];
        float* out1_tmp=new float[256*26*26];
        //compute
        this->backbone->forward(in,P4+128*26*26,feat2);                 //acts as a concat: P4 = concat(Upsample_out, feat1)
        //conv_forP5
        this->conv_forP5_conv->forward(feat2,P5);
        //out0
        this->yolo_headP5_basic_conv1->forward(P5,out0_tmp);
        conv(512,this->out_ch,0,1,1,13,13,out0_tmp,this->yolo_headP5_w2,this->yolo_headP5_b2,out0);
        //upsample
        this->upsample_conv->forward(P5,P5_Upsample);
        upsample(P5_Upsample,P4,13,13,128);
        //out1
        this->yolo_headP4_basic_conv1->forward(P4,out1_tmp);
        conv(256,this->out_ch,0,1,1,26,26,out1_tmp,this->yolo_headP4_w2,this->yolo_headP4_b2,out1);
        delete [] feat1;
        delete [] feat2;
        delete [] P5;
        delete [] out0_tmp;
        delete [] P5_Upsample;
        delete [] P4;
        delete [] out1_tmp;
    }
};


#endif // YOLO4_TINY_H_INCLUDED
