#include"pwconv_test.h"
#include"conv.h"

#define C_in 24
#define C_out 75
#define Nin 13
#define Nout Nin
#define ACT 1

data_t baseline_leakyrelu(data_t x){
	const data_t alpha=0.1;
	if(x<(data_t)0){
		return x*alpha;
	}
	else{
		return x;
	}
}

void conv1x1(data_t* in,data_t* w,data_t* b,data_t* out,int ch_in,int ch_out,int fsize,int act){
	for(int i=0;i<fsize;i++)
		for(int j=0;j<fsize;j++){
			for(int m=0;m<ch_out;m++){
				data_t tmp1=b[m];
				data_t tmp2;
				for(int n=0;n<ch_in;n++){
					tmp1+=in[n*fsize*fsize+i*fsize+j]*w[m*ch_in+n];
				}
				if(act==1)
				{
					tmp2=baseline_leakyrelu(tmp1);
				}
				else{
					tmp2=tmp1;
				}
				out[m*fsize*fsize+i*fsize+j]=tmp2;
			}
		}
}

void Pwconv_Test(){
	data_t in[C_in][Nin][Nin];
	data_t weight[C_out][C_in];
	data_t bias[C_out];
	data_t out[C_out][Nout][Nout];
	data_t exp_out[C_out][Nout][Nout];
    //
    int r,c,i,j,m,n;
    for(n=0;n<C_in;n++)
    	for(i=0;i<Nin;i++)
    		for(j=0;j<Nin;j++)
    		{
    			in[n][i][j]=(data_t)(((13*n*n*n+17*i*i*i*i+23*j*j+101)%512-256)/512.0);
    		}
    for(m=0;m<C_out;m++)
    	for(n=0;n<C_in;n++)
    		{
    			weight[m][n]=(data_t)(((m*m*m*19+n*n*13+17*m*n+41)%8192-4096)/4096.0);
    		}
    for(m=0;m<C_out;m++){
    	bias[m]=(data_t)(((m*m*m*19+m*m*7+19)%256-128)/128.0);
    }
	cout<<"start test"<<endl;
    //CONV BASELINE
    conv1x1((data_t*)in,(data_t*)weight,(data_t*)bias,(data_t*)exp_out,C_in,C_out,Nin,ACT);
    //TEST
	conv((data_t*)in,(data_t*)in,(data_t*)in,(data_t*)in,(data_t*)weight,(data_t*)weight,(data_t*)weight,(data_t*)weight,
		 (data_t*)bias,(data_t*)out,(data_t*)out,(data_t*)out,(data_t*)out,C_in,C_out,Nin,1,1,ACT);
	//compare
	cout<<"start compare"<<endl;
	for(int m=0;m<C_out;m++)
		for(int r=0;r<Nout;r++)
			for(int c=0;c<Nout;c++){
			    cout<<exp_out[m][r][c]<<","<<out[m][r][c]<<endl;
				if(exp_out[m][r][c]!=out[m][r][c]){
					cout<<"test failed!"<<endl;
					return;
				}
			}
	cout<<"test pass!"<<endl;
    return;
}


