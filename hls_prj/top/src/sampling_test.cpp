#include"sampling_test.h"

void pool(data_t* in,data_t* out,int ch,int fsize){
	    for(int n=0;n<ch;n++)
	        for(int i=0;i<fsize/2;i++)
	            for(int j=0;j<fsize/2;j++){
	                data_t tmp=in[n*fsize*fsize+(i*2)*fsize+(j*2)];
	                for(int kx=0;kx<2;kx++)
	                    for(int ky=0;ky<2;ky++){
	                        if(in[n*fsize*fsize+(i*2+kx)*fsize+(j*2+ky)]>tmp)
	                            tmp=in[n*fsize*fsize+(i*2+kx)*fsize+(j*2+ky)];
	                    }
	                out[n*(fsize*fsize/4)+i*(fsize/2)+j]=tmp;
	            }
}

void baseline_upsample(data_t* in,data_t* out,int h,int w,int ch){
    //ch x h x w-->ch x (2*h) x (2*w)
    for(int n=0;n<ch;n++)
        for(int i=0;i<h;i++)
            for(int j=0;j<w;j++){
                //out[n][2*i:2*i+2][2*j:2*j+2]=in[n][i][j]
                data_t tmp=in[n*h*w+i*w+j];
                out[n*(2*h)*(2*w)+(2*i+0)*(2*w)+(2*j+0)]=tmp;
                out[n*(2*h)*(2*w)+(2*i+0)*(2*w)+(2*j+1)]=tmp;
                out[n*(2*h)*(2*w)+(2*i+1)*(2*w)+(2*j+0)]=tmp;
                out[n*(2*h)*(2*w)+(2*i+1)*(2*w)+(2*j+1)]=tmp;
    }
}

void test1(){
	data_t in[128][13][13];
	data_t out[128][26][26];
	data_t out_ref[128][26][26];
	for(int i=0;i<13;i++)
		for(int j=0;j<13;j++)
			for(int n=0;n<128;n++){
				in[n][i][j]=(((n*n*n*71+n*n*13+i*i*43+j*29+(n+j+i)*61)%512-256)/256.0);
			}
	baseline_upsample((data_t*)in,(data_t*)out_ref,13,13,128);
	//sampling((data_t*)in,(data_t*)out,1,1,0);
	conv((data_t*)in,(data_t*)in,(data_t*)in,(data_t*)in,(data_t*)in,(data_t*)in,(data_t*)in,
	(data_t*)in,(data_t*)in,(data_t*)out,(data_t*)out,(data_t*)out,(data_t*)out,0,0,0,0,0,0);
	//
	for(int i=0;i<26;i++)
		for(int j=0;j<26;j++)
			for(int n=0;n<128;n++){
				cout<<out[n][i][j]<<","<<out_ref[n][i][j]<<endl;
				if(out[n][i][j]!=out_ref[n][i][j]){
					cout<<"error"<<endl;
					return;
				}
			}
	cout<<"right"<<endl;
	return;
}

void test2(){
	data_t in[C][N][N];
	data_t out[C][N/2][N/2];
	data_t out_ref[C][N/2][N/2];
	//
	for(int i=0;i<N;i++)
		for(int j=0;j<N;j++){
			for(int n=0;n<C;n++){
			    in[n][i][j]=(data_t)(((n*n*n*i*j+(i+j)*(n+i+j)+i*j+119)%512-256)/256.0);
		    }
		}
	//
	pool((data_t*)in,(data_t*)out_ref,C,N);
	//maxpool((data_t*)in,(data_t*)out,C,N,N);
	//sampling((data_t*)in,(data_t*)out,C,N,1);
	conv((data_t*)in,(data_t*)in,(data_t*)in,(data_t*)in,(data_t*)in,(data_t*)in,(data_t*)in,
		(data_t*)in,(data_t*)in,(data_t*)out,(data_t*)out,(data_t*)out,(data_t*)out,C,C,N,0,2,0);
	//
	for(int i=0;i<N/2;i++)
		for(int j=0;j<N/2;j++){
			for(int n=0;n<C;n++){
				cout<<out[n][i][j]<<","<<out_ref[n][i][j]<<endl;
			    if(out[n][i][j]!=out_ref[n][i][j]){
			    	cout<<"error"<<endl;
			    	return;
			    }
			}
		}
	cout<<"right"<<endl;
}

//int main(){
//	test1();
//}
