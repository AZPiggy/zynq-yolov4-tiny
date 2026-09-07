#include"pwconv.h"

data_t pwconv_leakyrelu(data_t x){
	const data_t alpha=(data_t)0.1;
	if(x>(data_t)0.0){
		return x;
	}
	else{
		return x*alpha;
	}
}

void load_input(data_t fm_in_buff[Tn][Tp],data_t* in1,data_t* in2,data_t* in3,data_t* in4,
		unsigned short n,unsigned short basePixAddr,unsigned short fm_size){
//load input tile In[n:n+Tn][basePixAddr:basePixAddr+Tp]
    unsigned short i;
    ap_uint<18> size=fm_size*fm_size;
//#pragma HLS RESOURCE variable=size core=Mul_LUT
    int base_addr=n*size+basePixAddr;
    for(i=0;i<Tp;i++){
#pragma HLS PIPELINE
    	fm_in_buff[0][i]=*(in1+base_addr+i);
    	fm_in_buff[1][i]=*(in2+base_addr+size+i);
    	fm_in_buff[2][i]=*(in3+base_addr+size*2+i);
    	fm_in_buff[3][i]=*(in4+base_addr+size*3+i);
    }
}

void load_weight(data_t wt_buff[Tm][Tn],data_t* weight,
		unsigned short n,unsigned short m,unsigned short ch_in,unsigned short ch_out){
//load weight tile W[m:m+Tm][n:n+Tn]
    unsigned short mm,nn;
    unsigned short num=((ch_out-m)<Tm)?(ch_out-m):Tm;
    for(mm=0;mm<num;mm++){
        for(int nn=0;nn<Tn;nn++){
#pragma HLS PIPELINE II=1
        	wt_buff[mm][nn]=*(weight+(m+mm)*ch_in+(n+nn));
        }
    }
}

void compute(data_t fm_in_buff[Tn][Tp],data_t fm_out_buff[Tm][Tp],data_t wt_buff[Tm][Tn],data_t bias_buff[Tm]){
#pragma HLS ARRAY_PARTITION variable=wt_buff complete dim=1
#pragma HLS ARRAY_PARTITION variable=wt_buff complete dim=2
#pragma HLS ARRAY_PARTITION variable=fm_out_buff complete dim=1
#pragma HLS ARRAY_PARTITION variable=fm_in_buff complete dim=1
    unsigned short i,nn,mm;
    data_t tmp;
	for(i=0;i<Tp;i++){
		#pragma HLS PIPELINE II=1
			for(mm=0;mm<Tm;mm++){
#pragma HLS UNROLL
					data_t mult1=fm_in_buff[0][i]*wt_buff[mm][0];
					data_t mult2=fm_in_buff[1][i]*wt_buff[mm][1];
					data_t mult3=fm_in_buff[2][i]*wt_buff[mm][2];
					data_t mult4=fm_in_buff[3][i]*wt_buff[mm][3];
					data_t sum1=mult1+mult2;
					data_t sum2=mult3+mult4;
					data_t sum=sum1+sum2;
					data_t accu=fm_out_buff[mm][i];
					fm_out_buff[mm][i]=accu+sum;
			}
		}
}

void load_bias(data_t fm_out_buff[Tm][Tp],data_t bias_buff[MAX_LEN/Tm][Tm],unsigned short m){
#pragma HLS INLINE off
#pragma HLS ARRAY_PARTITION variable=bias_buff complete dim=2
    unsigned short mm,i;
    for(i=0;i<Tp;i++){
#pragma HLS PIPELINE II=1
    		for(mm=0;mm<Tm;mm++){
                fm_out_buff[mm][i]=bias_buff[m/8][mm];
    		}
    	}
}

void store_output(data_t fm_out_buff[Tm][Tp],data_t* fm_out,unsigned short basePixAddr,
		          unsigned short m,unsigned short fm_size,unsigned short ch_out,int ACT){
//store fm_out_buff to Out[m:m+Tm][basePixAddr:basePixAddr+Tp]
    unsigned short i,mm;
    ap_uint<18> size=fm_size*fm_size;
//#pragma HLS RESOURCE variable=size core=Mul_LUT
    unsigned short num=((ch_out-m)<Tm)?(ch_out-m):Tm;
    for(mm=0;mm<num;mm++)
    	for(i=0;i<Tp;i++){
#pragma HLS PIPELINE II=1
    		data_t tmp1,tmp2;
    		tmp1=fm_out_buff[mm][i];
    		if(ACT==1){
    			tmp2=pwconv_leakyrelu(tmp1);
    		}
    		else{
    			tmp2=tmp1;
    		}
    		*(fm_out+(m+mm)*size+basePixAddr+i)=tmp2;
    	}
}


void compute_output(data_t fm_out_buff[Tm][Tp],data_t bias_buff[MAX_LEN/Tm][Tm],data_t* in1,data_t* in2,data_t* in3,data_t* in4,data_t* weight,
					unsigned short m,unsigned short fm_size,unsigned short ch_in,unsigned short ch_out,unsigned short basePixAddr){
#pragma HLS ARRAY_PARTITION variable=bias_buff complete dim=2
		data_t fm_in_buff1[Tn][Tp];
	#pragma HLS ARRAY_PARTITION variable=fm_in_buff1 complete dim=1
	    data_t fm_in_buff2[Tn][Tp];
	#pragma HLS ARRAY_PARTITION variable=fm_in_buff2 complete dim=1
	    data_t wt_buff1[Tm][Tn];
	#pragma HLS ARRAY_PARTITION variable=wt_buff1 complete dim=2
	#pragma HLS ARRAY_PARTITION variable=wt_buff1 complete dim=1
	    data_t wt_buff2[Tm][Tn];
	#pragma HLS ARRAY_PARTITION variable=wt_buff2 complete dim=2
	#pragma HLS ARRAY_PARTITION variable=wt_buff2 complete dim=1
		unsigned short ti=0;
//calc Out[m:m+Tm][basePixAddr:basePixAddr+Tp]
		load_bias(fm_out_buff,bias_buff,m);
		load_input(fm_in_buff1,in1,in2,in3,in4,ti,basePixAddr,fm_size);
		load_weight(wt_buff1,weight,ti,m,ch_in,ch_out);
		bool pingpong=true;             //
		for(ti=Tn;ti<ch_in;ti+=Tn){
#pragma HLS LOOP_TRIPCOUNT min=19 max=19 avg=19
			if(pingpong){          //
				load_input(fm_in_buff2,in1,in2,in3,in4,ti,basePixAddr,fm_size);
				load_weight(wt_buff2,weight,ti,m,ch_in,ch_out);
				compute(fm_in_buff1,fm_out_buff,wt_buff1,bias_buff[m/Tm]);
				pingpong=false;
			}
			else{
				load_input(fm_in_buff1,in1,in2,in3,in4,ti,basePixAddr,fm_size);
				load_weight(wt_buff1,weight,ti,m,ch_in,ch_out);
				compute(fm_in_buff2,fm_out_buff,wt_buff2,bias_buff[m/Tm]);
				pingpong=true;
			}
		}
		if(pingpong)
				compute(fm_in_buff1,fm_out_buff,wt_buff1,bias_buff[m/Tm]);
		else
				compute(fm_in_buff2,fm_out_buff,wt_buff2,bias_buff[m/Tm]);
}

void next_block(unsigned short fm_size,unsigned channel,
		        unsigned short basePixAddr,unsigned short baseChannel,
		        unsigned short &next_basePixAddr,unsigned short &next_baseChannel){
    if(basePixAddr+Tp>=fm_size*fm_size){
    	next_basePixAddr=0;
    	next_baseChannel=baseChannel+Tm;
    }
    else{
    	next_basePixAddr=basePixAddr+Tp;
    	next_baseChannel=baseChannel;
    }
}

void pwconv(data_t* in1,data_t* in2,data_t *in3,data_t* in4,data_t* weight,data_t* bias,
		    data_t* out,int N,int M,int SIZE,int ACT){
//
    unsigned short basePixAddr,baseChannel;
    unsigned short next_basePixAddr,next_baseChannel;
    data_t bias_buff[MAX_LEN/Tm][Tm];
#pragma HLS ARRAY_PARTITION variable=bias_buff complete dim=2
    data_t fm_out1[Tm][Tp];
#pragma HLS ARRAY_PARTITION variable=fm_out1 complete dim=1
    data_t fm_out2[Tm][Tp];
#pragma HLS ARRAY_PARTITION variable=fm_out2 complete dim=1
    //
    bool pingpong;
    memcpy((data_t*)bias_buff,(const data_t*)bias,sizeof(data_t)*M);
    //
    pingpong=true;
    basePixAddr=0;
    baseChannel=0;
    next_basePixAddr=Tp;
    next_baseChannel=0;
    compute_output(fm_out1,bias_buff,in1,in2,in3,in4,weight,baseChannel,SIZE,N,M,basePixAddr);
    while(true){
#pragma HLS LOOP_TRIPCOUNT min=119 max=119 avg=119
    	next_block(SIZE,M,basePixAddr,baseChannel,next_basePixAddr,next_baseChannel);
    	if(pingpong){
    		compute_output(fm_out2,bias_buff,in1,in2,in3,in4,weight,next_baseChannel,SIZE,N,M,next_basePixAddr);
    		store_output(fm_out1,out,basePixAddr,baseChannel,SIZE,M,ACT);
    		pingpong=false;
    	}
    	else{
    		compute_output(fm_out1,bias_buff,in1,in2,in3,in4,weight,next_baseChannel,SIZE,N,M,next_basePixAddr);
    		store_output(fm_out2,out,basePixAddr,baseChannel,SIZE,M,ACT);
    		pingpong=true;
    	}
    	basePixAddr=next_basePixAddr;
    	baseChannel=next_baseChannel;
    	if(basePixAddr+Tp>=SIZE*SIZE&&baseChannel+Tm>=M)
    		break;
    }
    if(pingpong){
		store_output(fm_out1,out,basePixAddr,baseChannel,SIZE,M,ACT);
    }
    else{
		store_output(fm_out2,out,basePixAddr,baseChannel,SIZE,M,ACT);
    }
}


