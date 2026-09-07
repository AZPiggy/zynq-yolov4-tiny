#include"basic_op.h"

void conv_leakyrelu(int ch_in,int ch_out,int pad,int stride,int k,int h,int w,data_t* in,data_t *weight,data_t *bias,data_t *out,int act){
	//Xil_DCacheFlushRange((u32)in,sizeof(data_t)*ch_in*h*w);//flush the input buffer out to DDR
	XConv_Set_in1_V(&conv_inst, (u32)in);
	XConv_Set_in2_V(&conv_inst, (u32)in);
	XConv_Set_in3_V(&conv_inst, (u32)in);
	XConv_Set_in4_V(&conv_inst, (u32)in);

	XConv_Set_w1_V(&conv_inst, (u32)weight);
	XConv_Set_w2_V(&conv_inst, (u32)weight);
	XConv_Set_w3_V(&conv_inst, (u32)weight);
	XConv_Set_w4_V(&conv_inst, (u32)weight);

	XConv_Set_b_V(&conv_inst, (u32)bias);
	XConv_Set_out1_V(&conv_inst, (u32)out);
	XConv_Set_out2_V(&conv_inst, (u32)out);
	XConv_Set_out3_V(&conv_inst, (u32)out);
	XConv_Set_out4_V(&conv_inst, (u32)out);

	XConv_Set_ch_in(&conv_inst,ch_in);
	XConv_Set_ch_out(&conv_inst,ch_out);
	XConv_Set_fsize(&conv_inst,h);
	XConv_Set_stride(&conv_inst,stride);
	XConv_Set_kernel(&conv_inst,k);
	XConv_Set_act(&conv_inst,act);
	//
	XConv_Start(&conv_inst);
	while(XConv_IsDone(&conv_inst)==0);
//	if(k==3&&stride==2)
//		Xil_DCacheInvalidateRange((u32)((unsigned int)out&0xffffffe0), 32*((ch_out*h*w/4*sizeof(data_t))/32+2));
//	else
//	    Xil_DCacheInvalidateRange((u32)((unsigned int)out&0xffffffe0), 32*((ch_out*h*w*sizeof(data_t))/32+2));

}

void conv_init(){
	XConv_Initialize(&conv_inst, 0);
}

void sampling(data_t* in,data_t* out,int ch,int fsize,int mode){
	if(mode==1)   //maxpool
	    conv_leakyrelu(ch,ch,0,0,2,fsize,fsize,in,in,in,out,0);
	else
		conv_leakyrelu(ch,ch,0,0,0,fsize,fsize,in,in,in,out,0);
}
