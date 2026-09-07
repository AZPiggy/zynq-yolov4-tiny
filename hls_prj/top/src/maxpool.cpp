#include"maxpool.h"
#define Tr 13
#define Tc 13
#define Tn 4

void load_input(data_t* in,data_t fm_in_buff[Tn][2*Tr][2*Tc],unsigned short n,unsigned short r,unsigned short c,unsigned short fsize,unsigned short size){
	//load in[n:n+Tn][r*2:r*2+Tr*2][c*2:c*2+Tc*2]
	//unsigned short size=fsize*fsize;
	int n_base=n*size;
	int r_base=2*r*fsize;
//#pragma HLS RESOURCE variable=r_base core=Mul_LUT
	int c_base=2*c;
//#pragma HLS RESOURCE variable=c_base core=Mul_LUT
	int base_addr=n_base+r_base+c_base;
    //
	for(unsigned short nn=0;nn<Tn;nn++){
		for(unsigned short ii=0;ii<Tr*2;ii++){
			for(unsigned short jj=0;jj<Tc*2;jj++){
#pragma HLS PIPELINE II=1
				int nn_size=nn*size;
#pragma HLS RESOURCE variable=nn_size core=Mul_LUT
				int ii_fsize=ii*fsize;
#pragma HLS RESOURCE variable=ii_fsize core=Mul_LUT
				fm_in_buff[nn][ii][jj]=*(in+base_addr+nn_size+ii_fsize+jj);
				//fm_in_buff[nn][ii][jj]=*(in+(n+nn)*fsize*fsize+(ii+r*2)*fsize+(jj+c*2));
			}
		}
	}
}

void store_output(data_t* out,data_t fm_out_buff[Tn][Tr][Tc],unsigned short n,unsigned short r,unsigned short c,unsigned short fsize,unsigned short size){
	//unsigned short size=fsize*fsize/4;
	int n_base=n*size;
	int r_base=r*fsize/2;
	int base_addr=n_base+r_base+c;
    //
	for(unsigned short nn=0;nn<Tn;nn++)
		for(unsigned short ii=0;ii<Tr;ii++)
			for(unsigned short jj=0;jj<Tc;jj++){
#pragma HLS PIPELINE II=1
				int nn_size=nn*size;
#pragma HLS RESOURCE variable=nn_size core=Mul_LUT
				int ii_fsize=ii*fsize;
#pragma HLS RESOURCE variable=ii_fsize core=Mul_LUT
				*(out+base_addr+nn_size+ii_fsize/2+jj)=fm_out_buff[nn][ii][jj];
			}
}

void compute(data_t fm_in_buff[Tn][Tr*2][Tc*2],data_t fm_out_buff[Tn][Tr][Tc]){
	data_t tmp1,tmp2,tmp3,tmp4;
	data_t max1,max2,max;
	for(unsigned short n=0;n<Tn;n++)
		for(unsigned short i=0;i<Tr;i++)
			for(unsigned short j=0;j<Tc;j++){
#pragma HLS PIPELINE
				tmp1=fm_in_buff[n][i*2][j*2];
				tmp2=fm_in_buff[n][i*2][j*2+1];
				tmp3=fm_in_buff[n][i*2+1][j*2];
				tmp4=fm_in_buff[n][i*2+1][j*2+1];
				max1=(tmp1>tmp2)?tmp1:tmp2;
				max2=(tmp3>tmp4)?tmp3:tmp4;
				max=(max1>max2)?max1:max2;
				fm_out_buff[n][i][j]=max;
			}
}


void maxpool(data_t* in,data_t* out,int ch,int h,int w){
//#pragma HLS INTERFACE s_axilite port=return bundle=CTRL
//#pragma HLS INTERFACE s_axilite port=w bundle=CTRL
//#pragma HLS INTERFACE s_axilite port=ch bundle=CTRL
//#pragma HLS INTERFACE s_axilite port=h bundle=CTRL
//#pragma HLS INTERFACE m_axi depth=2704 port=out offset=slave bundle=OUT
//#pragma HLS INTERFACE m_axi depth=10816 port=in offset=slave bundle=IN
	data_t fm_in_buff[Tn][Tr*2][Tc*2];
	data_t fm_out_buff[Tn][Tr][Tc];
	unsigned short size=h*w;
	for(unsigned short n=0;n<ch;n+=Tn)
		for(unsigned short i=0;i<h/2;i+=Tr)
			for(unsigned short j=0;j<w/2;j+=Tc){
				load_input(in,fm_in_buff,n,i,j,h,size);
				compute(fm_in_buff,fm_out_buff);
				store_output(out,fm_out_buff,n,i,j,h,size/4);
			}
}
