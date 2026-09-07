#include"debug.h"
#include"basic_block.h"
#include"image_io.h"
#include"yolo_postprocess.h"
#include<iostream>
#include "xtime_l.h"
using namespace std;

#ifdef QIMING_DIAGNOSTIC
extern "C" volatile unsigned int g_diag_stage;
extern "C" volatile int g_diag_detection_count;
extern "C" volatile int g_diag_out0_min;
extern "C" volatile int g_diag_out0_max;
extern "C" volatile unsigned int g_diag_out0_nonzero;
extern "C" volatile int g_diag_out1_min;
extern "C" volatile int g_diag_out1_max;
extern "C" volatile unsigned int g_diag_out1_nonzero;
extern "C" volatile unsigned int g_diag_inference_us;
#define SET_DIAG_STAGE(value) (g_diag_stage = (value))
#else
#define SET_DIAG_STAGE(value) ((void)0)
#endif

bool yolo_test(string weight_dir, string input_image, string output_jpeg){
    SET_DIAG_STAGE(0x10);
    RgbImage image;
    if (!load_image_from_sd(input_image.c_str(), &image)) {
        return false;
    }
    SET_DIAG_STAGE(0x11);

    data_t* in=new data_t[3*416*416];
    data_t* out0=new data_t[OUT_CH*13*13];
    data_t* out1=new data_t[OUT_CH*26*26];
    XTime tEnd, tCur;
    u32 tUsed;

    cout << "preprocessing image" << endl;
    preprocess_rgb(image, in);
    SET_DIAG_STAGE(0x20);
    conv_init();
    Yolo4_Tiny* yolo4_tiny=new Yolo4_Tiny();
    SET_DIAG_STAGE(0x30);
    yolo4_tiny->load_weight(weight_dir);
    SET_DIAG_STAGE(0x31);
    Xil_DCacheFlush();
    XTime_GetTime(&tCur);
    SET_DIAG_STAGE(0x40);
    yolo4_tiny->forward(in,out0,out1);
    SET_DIAG_STAGE(0x41);
    Xil_DCacheInvalidateRange((u32)((unsigned int)out0&0xffffffe0), 32*((OUT_CH*13*13*sizeof(data_t))/32+2));
    Xil_DCacheInvalidateRange((u32)((unsigned int)out1&0xffffffe0), 32*((OUT_CH*26*26*sizeof(data_t))/32+2));
    XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	cout<<"time elapsed is "<<tUsed<<" us"<<endl;

#ifdef QIMING_DIAGNOSTIC
    g_diag_inference_us = tUsed;
    g_diag_out0_min = g_diag_out0_max = out0[0];
    for (int i = 0; i < OUT_CH * 13 * 13; ++i) {
        if (out0[i] < g_diag_out0_min) g_diag_out0_min = out0[i];
        if (out0[i] > g_diag_out0_max) g_diag_out0_max = out0[i];
        if (out0[i] != 0) ++g_diag_out0_nonzero;
    }
    g_diag_out1_min = g_diag_out1_max = out1[0];
    for (int i = 0; i < OUT_CH * 26 * 26; ++i) {
        if (out1[i] < g_diag_out1_min) g_diag_out1_min = out1[i];
        if (out1[i] > g_diag_out1_max) g_diag_out1_max = out1[i];
        if (out1[i] != 0) ++g_diag_out1_nonzero;
    }
#endif

    const int MAX_DETECTIONS = 100;
    Detection detections[MAX_DETECTIONS];
    const int detection_count = decode_yolo_outputs(out0, out1, image.width, image.height,
                                                     detections, MAX_DETECTIONS);
#ifdef QIMING_DIAGNOSTIC
    g_diag_detection_count = detection_count;
#endif
    SET_DIAG_STAGE(0x50);
    cout << "detections after NMS: " << detection_count << endl;
    for (int i = 0; i < detection_count; ++i) {
        cout << "class=" << detections[i].class_id << " score=" << detections[i].score
             << " box=(" << detections[i].left << "," << detections[i].top << ","
             << detections[i].right << "," << detections[i].bottom << ")" << endl;
    }
    draw_detections(&image, detections, detection_count);
    SET_DIAG_STAGE(0x60);
    const bool saved = save_jpeg_to_sd(output_jpeg.c_str(), image, 90);
    SET_DIAG_STAGE(saved ? 0x61 : 0xE060);

    delete [] in;
    delete [] out0;
    delete [] out1;
    free_rgb_image(&image);
    return saved;
}
