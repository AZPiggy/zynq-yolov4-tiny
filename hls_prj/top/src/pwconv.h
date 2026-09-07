#include<ap_fixed.h>
#include<string>
#define Tp 169                   //13*13
#define Tn 4
#define Tm 8
#define MAX_LEN 1024
typedef ap_fixed<16,7,AP_RND,AP_SAT> data_t;
//
void pwconv(data_t* in1,data_t* in2,data_t* in3,data_t* in4,data_t* weight,data_t* bias,
		data_t* out,int N,int M,int SIZE,int ACT);



