/*
 * sd_io.c
 *
 *  Created on: 2021-08-18
 *      Author: DELL
 */
#include "sd_io.h"

#ifdef QIMING_DIAGNOSTIC
extern "C" volatile unsigned int g_diag_param_read_count;
extern "C" volatile unsigned int g_diag_param_read_failures;
extern "C" volatile unsigned int g_diag_param_bytes;
#endif


int SD_Init()
{
    FRESULT rc;

    rc = f_mount(&fatfs,"",0);
    if(rc)
    {
        xil_printf("ERROR : f_mount returned %d\r\n",rc);
        return XST_FAILURE;
    }
    return XST_SUCCESS;
}

int SD_Transfer_read(char *FileName,u32 DestinationAddress,u32 ByteLength)
{
    FIL fil;
    FRESULT rc;
    UINT br;

    rc = f_open(&fil,FileName,FA_READ);
    if(rc)
    {
        xil_printf("ERROR : f_open returned %d\r\n",rc);
        return XST_FAILURE;
    }
    rc = f_lseek(&fil, 0);
    if(rc)
    {
        xil_printf("ERROR : f_lseek returned %d\r\n",rc);
        return XST_FAILURE;
    }
    rc = f_read(&fil, (void*)DestinationAddress,ByteLength,&br);
    if(rc)
    {
        xil_printf("ERROR : f_read returned %d\r\n",rc);
        return XST_FAILURE;
    }
    if(br != ByteLength)
    {
        xil_printf("ERROR : short read %d/%d bytes\r\n",br,ByteLength);
        f_close(&fil);
        return XST_FAILURE;
    }
    rc = f_close(&fil);
    if(rc)
    {
        xil_printf(" ERROR : f_close returned %d\r\n", rc);
        return XST_FAILURE;
    }
    return XST_SUCCESS;
}

int SD_Transfer_write(char *FileName,u32 SourceAddress,u32 ByteLength)
{
    FIL fil;
    FRESULT rc;
    UINT bw;

    rc = f_open(&fil,FileName,FA_CREATE_ALWAYS | FA_WRITE);
    if(rc)
    {
        xil_printf("ERROR : f_open returned %d\r\n",rc);
        return XST_FAILURE;
    }
    rc = f_lseek(&fil, 0);
    if(rc)
    {
        xil_printf("ERROR : f_lseek returned %d\r\n",rc);
        return XST_FAILURE;
    }
    rc = f_write(&fil,(void*) SourceAddress,ByteLength,&bw);
    if(rc)
    {
        xil_printf("ERROR : f_write returned %d\r\n", rc);
        return XST_FAILURE;
    }
    if(bw != ByteLength)
    {
        xil_printf("ERROR : short write %d/%d bytes\r\n",bw,ByteLength);
        f_close(&fil);
        return XST_FAILURE;
    }
    rc = f_close(&fil);
    if(rc){
        xil_printf("ERROR : f_close returned %d\r\n",rc);
        return XST_FAILURE;
    }
    return XST_SUCCESS;
}


void read_params(string filename,data_t* x,int length){
	cout<<"reading "<<filename<<endl;
#ifdef QIMING_DIAGNOSTIC
    ++g_diag_param_read_count;
#endif
    float* tmp=new float[length];
	if(SD_Transfer_read((char*)(filename.c_str()),(u32)tmp,(u32)(sizeof(float)*length)) != XST_SUCCESS){
#ifdef QIMING_DIAGNOSTIC
        ++g_diag_param_read_failures;
#endif
		delete [] tmp;
		return;
	}
#ifdef QIMING_DIAGNOSTIC
    g_diag_param_bytes += static_cast<unsigned int>(sizeof(float) * length);
#endif
    for(int i=0;i<length;i++){
        x[i]=(data_t)(round(tmp[i]*SCALE));
    }
    delete [] tmp;
}

void write_data(string filename,data_t* x,int length){
	cout<<"writing "<<filename<<endl;
	float* tmp=new float[length];
	for(int i=0;i<length;i++){
		tmp[i]=(float)x[i]/(float)SCALE;
	}
	SD_Transfer_write((char*)filename.c_str(),(u32)tmp,(u32)(sizeof(float)*length));
	delete [] tmp;
}




