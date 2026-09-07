#include"conv.h"
#include"winoconv_test.h"
#include<iostream>
using namespace std;

data_t baselineleakyrelu(data_t x){
	const data_t alpha=(data_t)0.1;
	if(x>=(data_t)0){
		return x;
	}
	else{
		return x*alpha;
	}
}

void baseline_conv(data_t* in,data_t* weight,data_t* bias,data_t* out,int h,int w,int stride,int act,int ch_in,int ch_out){
	    int h_o=h/stride;
	    int w_o=w/stride;
	    for(int r=0;r<h/stride;r++)
	        for(int c=0;c<w/stride;c++)
	            for(int m=0;m<ch_out;m++)
	            {
	                data_t tmp=bias[m];
	                for(int n=0;n<ch_in;n++)
	                    for(int i=0;i<3;i++)
	                        for(int j=0;j<3;j++){
	                            if((r*stride+i-1)>=0&&(r*stride+i-1)<h&&(c*stride+j-1)>=0&&(c*stride+j-1)<w)
	                                tmp+=(in[n*h*w+(r*stride+i-1)*w+(c*stride+j-1)]*weight[m*ch_in*9+n*9+i*3+j]);
	                        }
	                if(act==1){
	                	out[m*h_o*w_o+r*w_o+c]=baselineleakyrelu(tmp);
	                }
	                else{
	                	out[m*h_o*w_o+r*w_o+c]=tmp;
	                }
	            }
}

void StdConvTest(int ch_in,int ch_out,int fsize,int stride,int act) {
	data_t* bias=(data_t*)malloc(ch_out*sizeof(data_t));
	data_t *in=(data_t*)malloc(ch_in*fsize*fsize*sizeof(data_t));
	data_t *weight = (data_t*)malloc(ch_out*ch_in*9*sizeof(data_t));
	data_t *out1=(data_t*)malloc((ch_out*fsize*fsize+1000000)*sizeof(data_t));
	data_t *out2=(data_t*)malloc((ch_out*fsize*fsize+1000000)*sizeof(data_t));
	//initialise
	for(int i=0;i<ch_out;i++){
		bias[i]=(rand()%256-128)/128.0;
	}
	for (int i = 0; i < ch_in*fsize*fsize; i++)
		*((data_t*)in + i) = (rand()% 128 - 64)/128.0;
	for (int i = 0; i < ch_out * ch_in * 9; i++)
		*((data_t*)weight + i) = (rand()% 128 - 64)/128.0;
	//compute
	baseline_conv((data_t*)in, (data_t*)weight, (data_t*)bias,(data_t*)out1, fsize, fsize,stride,act,ch_in, ch_out);
	conv((data_t*)in,(data_t*)in,(data_t*)in,(data_t*)in,(data_t*)weight,(data_t*)weight,(data_t*)weight,(data_t*)weight,
		 (data_t*)bias,(data_t*)out2,(data_t*)out2,(data_t*)out2,(data_t*)out2,ch_in,ch_out,fsize,stride,3,act);
	//
	int total_num=ch_out*(fsize/stride)*(fsize/stride);
	for (int i = 0; i < total_num; i++) {
		cout << *((data_t*)out1 + i) <<","<<*((data_t*)out2 + i) << endl;
	}
	free(in);
	free(bias);
	free(weight);
	free(out1);
	free(out2);
	return;
}

