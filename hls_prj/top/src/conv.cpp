#include"conv.h"



void conv(data_t* in1,data_t* in2,data_t* in3,data_t* in4,data_t* w1,data_t* w2,data_t* w3,
data_t* w4,data_t* b,data_t* out1,data_t* out2,data_t* out3,data_t* out4,
int ch_in,int ch_out,int fsize,int stride,int kernel,int act){
//PRAGMA
#pragma HLS INTERFACE s_axilite port=return bundle=CTRL
#pragma HLS INTERFACE s_axilite port=fsize bundle=CTRL
#pragma HLS INTERFACE s_axilite port=ch_out bundle=CTRL
#pragma HLS INTERFACE s_axilite port=act bundle=CTRL
#pragma HLS INTERFACE s_axilite port=stride bundle=CTRL
#pragma HLS INTERFACE s_axilite port=ch_in bundle=CTRL
#pragma HLS INTERFACE s_axilite port=kernel bundle=CTRL
#pragma HLS INTERFACE m_axi depth=60000 port=in1 offset=slave bundle=FM1 max_read_burst_length=256 max_write_burst_length=256
#pragma HLS INTERFACE m_axi depth=60000 port=in2 offset=slave bundle=FM2 max_read_burst_length=256 max_write_burst_length=256
#pragma HLS INTERFACE m_axi depth=60000 port=in3 offset=slave bundle=FM3 max_read_burst_length=256 max_write_burst_length=256
#pragma HLS INTERFACE m_axi depth=60000 port=in4 offset=slave bundle=FM4 max_read_burst_length=256 max_write_burst_length=256
#pragma HLS INTERFACE m_axi depth=65536 port=w1 offset=slave bundle=W1 max_read_burst_length=256
#pragma HLS INTERFACE m_axi depth=65536 port=w2 offset=slave bundle=W2 max_read_burst_length=256
#pragma HLS INTERFACE m_axi depth=65536 port=w3 offset=slave bundle=W3 max_read_burst_length=256
#pragma HLS INTERFACE m_axi depth=65536 port=w4 offset=slave bundle=W4 max_read_burst_length=256
#pragma HLS INTERFACE m_axi depth=128 port=b offset=slave bundle=W1 max_read_burst_length=256
#pragma HLS INTERFACE m_axi port=out1 offset=slave bundle=FM1
#pragma HLS INTERFACE m_axi port=out2 offset=slave bundle=FM2
#pragma HLS INTERFACE m_axi port=out3 offset=slave bundle=FM3
#pragma HLS INTERFACE m_axi port=out4 offset=slave bundle=FM4
//content
	if(kernel==3){
		//WinogradConv(in1,in2,in3,in4,w1,w2,w3,w4,b,out1,out2,ch_out,ch_in,fsize,stride,act);
		std_conv(in1,in2,in3,in4,w1,w2,w3,w4,b,out1,out2,out3,out4,ch_in,ch_out,fsize,stride,act);
	}
	else if(kernel==1){
		pwconv(in1,in2,in3,in4,w1,b,out1,ch_in,ch_out,fsize,act);
	}
	else if(kernel==2){     //kernel==2 maxpool; kernel==0 upsample
		maxpool(in1,out1,ch_in,fsize,fsize);
	}
	else{
		upsample(in1,out1);
	}
}
