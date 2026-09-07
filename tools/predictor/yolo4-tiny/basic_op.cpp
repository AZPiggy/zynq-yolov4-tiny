#include"basic_op.h"

void conv(int ch_in,int ch_out,int pad,int stride,int k,int h,int w,
          float* in,float *weight,float *bias,float *out){
    int h_o,w_o;
    int i,j,n,m;
    int kx,ky;
    h_o=(h-k+2*pad)/stride+1;
    w_o=(w-k+2*pad)/stride+1;
    //cout<<"h_o="<<h_o<<endl;
    //cout<<"w_o="<<w_o<<endl;
    //cout<<"ch_out="<<ch_out<<endl;
    for(i=0;i<h_o;i++)
        for(j=0;j<w_o;j++)
            for(m=0;m<ch_out;m++){
                //
                int addr_o=m*h_o*w_o+i*w_o+j;
                *(out+addr_o)=bias[m];
                for(n=0;n<ch_in;n++)
                    for(kx=0;kx<k;kx++)
                        for(ky=0;ky<k;ky++)
                    {
                    //out[m][i][j]+=in[n][i*stride+kx-pad][j*stride+ky-pad]*weight[m][n][kx][ky]
                        int row=i*stride+kx-pad;
                        int col=j*stride+ky-pad;
                        int addr_i=n*h*w+row*w+col;
                        //
                        int addr_w=m*ch_in*k*k+n*k*k+kx*k+ky;
                        if(row>=0&&row<h&&col>=0&&col<w)
                            *(out+addr_o)+=(*(in+addr_i))*(*(weight+addr_w));
                    }
    }
}

void maxpool(float* in,float* out,int ch,int h,int w)                     //2x2 maxpooling
{
    int h_o=h/2;
    int w_o=w/2;
    for(int n=0;n<ch;n++){
        for(int i=0;i<h_o;i++)
                for(int j=0;j<w_o;j++)
                    {
                        float tmp=-9999.0;
                        for(int kx=0;kx<2;kx++)
                            for(int ky=0;ky<2;ky++)
                            {
                                int r=i*2+kx;
                                int c=j*2+ky;
                                if(r>=0&&r<h&&c>=0&&c<w)
                                    tmp=((*(in+n*h*w+r*w+c))>tmp)?(*(in+n*h*w+r*w+c)):tmp;
                            }
                        *(out+n*h_o*w_o+i*w_o+j)=tmp;
                    }
    }
}

void leakyrelu(float* in,float* out,int ch,int h,int w,float a){
    int num=h*w*ch;
    for(int i=0;i<num;i++){
          if(in[i]>=0){
                out[i]=in[i];
          }
          else{
               out[i]=in[i]*a;
          }
    }
}

void read_params(string filename,float* x,int length){
    cout<<"reading "<<filename<<endl;
    FILE* fp=fopen(filename.c_str(),"rb");
    if(fp!=NULL){
          fread(x,sizeof(float),length,fp);
    }
    else{
          cout<<"open "<<filename<<" error"<<endl;
    }
    fclose(fp);
}

void upsample(float* in,float* out,int h,int w,int ch){
    //ch x h x w-->ch x (2*h) x (2*w)
    for(int n=0;n<ch;n++)
        for(int i=0;i<h;i++)
            for(int j=0;j<w;j++){
                //out[n][2*i:2*i+2][2*j:2*j+2]=in[n][i][j]
                float tmp=in[n*h*w+i*w+j];
                out[n*(2*h)*(2*w)+(2*i+0)*(2*w)+(2*j+0)]=tmp;
                out[n*(2*h)*(2*w)+(2*i+0)*(2*w)+(2*j+1)]=tmp;
                out[n*(2*h)*(2*w)+(2*i+1)*(2*w)+(2*j+0)]=tmp;
                out[n*(2*h)*(2*w)+(2*i+1)*(2*w)+(2*j+1)]=tmp;
    }
}
