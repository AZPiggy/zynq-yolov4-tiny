#ifndef YOLO_POSTPROCESS_H_INCLUDED
#define YOLO_POSTPROCESS_H_INCLUDED

#include "image_io.h"
#include "sd_io.h"

struct Detection {
    float left;
    float top;
    float right;
    float bottom;
    float score;
    int class_id;
};

void preprocess_rgb(const RgbImage& image, data_t* network_input);
int decode_yolo_outputs(const data_t* out0, const data_t* out1,
                        int image_width, int image_height,
                        Detection* detections, int max_detections);
void draw_detections(RgbImage* image, const Detection* detections, int count);

#endif
