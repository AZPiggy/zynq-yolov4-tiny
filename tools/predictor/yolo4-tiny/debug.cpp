#include"debug.h"
#include"basic_block.h"
#include<iostream>
using namespace std;

void write_result(string file,float* x,int length){
      FILE* fp=fopen(file.c_str(),"wb");
      if(fp==NULL){
          printf("error open %s",file.c_str());
      }
      else{
            fwrite(x,sizeof(float),length,fp);
      }
}

void yolo_test(string weight_dir,string ref_dir){
    int class_num=80;
    int ch_out=(4+1+class_num)*3;
    float* in=new float[3*416*416];
    float* out0=new float[ch_out*13*13];
    float* out1=new float[ch_out*26*26];
    //
    read_params(ref_dir+"\\img.bin",in,3*416*416);
    //
    Yolo4_Tiny* yolo4_tiny=new Yolo4_Tiny();
    yolo4_tiny->load_weight(weight_dir);
    yolo4_tiny->forward(in,out0,out1);
    //
    write_result("..\\yolo_test\\out0.bin",out0,ch_out*13*13);
    write_result("..\\yolo_test\\out1.bin",out1,ch_out*26*26);
    delete [] in;
    delete [] out0;
    delete [] out1;
    return;
}


