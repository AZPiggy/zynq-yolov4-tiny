#include "yolo_postprocess.h"

#include <algorithm>
#include <cmath>

#ifdef QIMING_DIAGNOSTIC
extern "C" volatile unsigned int g_diag_head_max_objectness[2];
extern "C" volatile unsigned int g_diag_head_max_score[2];
extern "C" volatile int g_diag_head_best_class[2];
extern "C" volatile unsigned int g_diag_cat_max_score;
extern "C" volatile unsigned int g_diag_dog_max_score;
#endif

namespace {

const int INPUT_SIZE = 416;
const int NUM_CLASSES = 80;
const int ATTRS = NUM_CLASSES + 5;
const int MAX_CANDIDATES = 2048;
const float CONFIDENCE_THRESHOLD = 0.20f;
const float NMS_IOU_THRESHOLD = 0.3f;

const float ANCHORS[6][2] = {
    {10.0f, 14.0f}, {23.0f, 27.0f}, {37.0f, 58.0f},
    {81.0f, 82.0f}, {135.0f, 169.0f}, {344.0f, 319.0f}
};

const int ANCHOR_MASKS[2][3] = {{3, 4, 5}, {1, 2, 3}};

float sigmoid(float value)
{
    if (value >= 0.0f) {
        const float z = static_cast<float>(std::exp(-value));
        return 1.0f / (1.0f + z);
    }
    const float z = static_cast<float>(std::exp(value));
    return z / (1.0f + z);
}

float clampf(float value, float low, float high)
{
    return value < low ? low : (value > high ? high : value);
}

float tensor_value(const data_t* tensor, int grid, int channel, int y, int x)
{
    return static_cast<float>(tensor[(channel * grid + y) * grid + x]) / static_cast<float>(SCALE);
}

float intersection_over_union(const Detection& a, const Detection& b)
{
    const float left = a.left > b.left ? a.left : b.left;
    const float top = a.top > b.top ? a.top : b.top;
    const float right = a.right < b.right ? a.right : b.right;
    const float bottom = a.bottom < b.bottom ? a.bottom : b.bottom;
    const float width = right - left;
    const float height = bottom - top;
    if (width <= 0.0f || height <= 0.0f) {
        return 0.0f;
    }
    const float intersection = width * height;
    const float area_a = (a.right - a.left) * (a.bottom - a.top);
    const float area_b = (b.right - b.left) * (b.bottom - b.top);
    return intersection / (area_a + area_b - intersection);
}

struct HigherScore {
    bool operator()(const Detection& a, const Detection& b) const { return a.score > b.score; }
};

void decode_head(const data_t* output, int grid, int head_index,
                 int image_width, int image_height,
                 Detection* candidates, int* candidate_count)
{
    for (int anchor = 0; anchor < 3; ++anchor) {
        const int base = anchor * ATTRS;
        const int anchor_index = ANCHOR_MASKS[head_index][anchor];
        for (int y = 0; y < grid; ++y) {
            for (int x = 0; x < grid; ++x) {
                const float objectness = sigmoid(tensor_value(output, grid, base + 4, y, x));
#ifdef QIMING_DIAGNOSTIC
                const unsigned int objectness_scaled = static_cast<unsigned int>(objectness * 1000000.0f);
                if (objectness_scaled > g_diag_head_max_objectness[head_index]) {
                    g_diag_head_max_objectness[head_index] = objectness_scaled;
                }
                const unsigned int cat_score_scaled = static_cast<unsigned int>(
                    objectness * sigmoid(tensor_value(output, grid, base + 5 + 15, y, x)) * 1000000.0f);
                const unsigned int dog_score_scaled = static_cast<unsigned int>(
                    objectness * sigmoid(tensor_value(output, grid, base + 5 + 16, y, x)) * 1000000.0f);
                if (cat_score_scaled > g_diag_cat_max_score) g_diag_cat_max_score = cat_score_scaled;
                if (dog_score_scaled > g_diag_dog_max_score) g_diag_dog_max_score = dog_score_scaled;
#endif
                if (objectness < CONFIDENCE_THRESHOLD) {
                    continue;
                }

                int class_id = 0;
                float class_confidence = 0.0f;
                for (int c = 0; c < NUM_CLASSES; ++c) {
                    const float confidence = sigmoid(tensor_value(output, grid, base + 5 + c, y, x));
                    if (confidence > class_confidence) {
                        class_confidence = confidence;
                        class_id = c;
                    }
                }
                const float score = objectness * class_confidence;
#ifdef QIMING_DIAGNOSTIC
                const unsigned int score_scaled = static_cast<unsigned int>(score * 1000000.0f);
                if (score_scaled > g_diag_head_max_score[head_index]) {
                    g_diag_head_max_score[head_index] = score_scaled;
                    g_diag_head_best_class[head_index] = class_id;
                }
#endif
                if (score < CONFIDENCE_THRESHOLD || *candidate_count >= MAX_CANDIDATES) {
                    continue;
                }

                const float center_x = (sigmoid(tensor_value(output, grid, base, y, x)) + x) / grid;
                const float center_y = (sigmoid(tensor_value(output, grid, base + 1, y, x)) + y) / grid;
                const float box_w = static_cast<float>(std::exp(clampf(tensor_value(output, grid, base + 2, y, x), -10.0f, 10.0f))) *
                                    ANCHORS[anchor_index][0] / INPUT_SIZE;
                const float box_h = static_cast<float>(std::exp(clampf(tensor_value(output, grid, base + 3, y, x), -10.0f, 10.0f))) *
                                    ANCHORS[anchor_index][1] / INPUT_SIZE;

                Detection& detection = candidates[(*candidate_count)++];
                detection.left = clampf((center_x - box_w * 0.5f) * image_width, 0.0f, image_width - 1.0f);
                detection.top = clampf((center_y - box_h * 0.5f) * image_height, 0.0f, image_height - 1.0f);
                detection.right = clampf((center_x + box_w * 0.5f) * image_width, 0.0f, image_width - 1.0f);
                detection.bottom = clampf((center_y + box_h * 0.5f) * image_height, 0.0f, image_height - 1.0f);
                detection.score = score;
                detection.class_id = class_id;
            }
        }
    }
}

void set_pixel(RgbImage* image, int x, int y, const unsigned char color[3])
{
    if (x < 0 || y < 0 || x >= image->width || y >= image->height) {
        return;
    }
    unsigned char* pixel = image->pixels + (y * image->width + x) * 3;
    pixel[0] = color[0];
    pixel[1] = color[1];
    pixel[2] = color[2];
}

void draw_class_id(RgbImage* image, int x, int y, int class_id, const unsigned char color[3])
{
    static const unsigned char digits[10][5] = {
        {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
        {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7}
    };
    int values[2] = {class_id / 10, class_id % 10};
    int first = class_id >= 10 ? 0 : 1;
    for (int digit_index = first; digit_index < 2; ++digit_index) {
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 3; ++col) {
                if (digits[values[digit_index]][row] & (1U << (2 - col))) {
                    set_pixel(image, x + (digit_index - first) * 4 + col, y + row, color);
                }
            }
        }
    }
}

void fill_rectangle(RgbImage* image, int left, int top, int right, int bottom, const unsigned char color[3])
{
    for (int y = top; y <= bottom; ++y) {
        for (int x = left; x <= right; ++x) {
            set_pixel(image, x, y, color);
        }
    }
}

} // namespace

void preprocess_rgb(const RgbImage& image, data_t* network_input)
{
    const int plane = INPUT_SIZE * INPUT_SIZE;
    for (int dst_y = 0; dst_y < INPUT_SIZE; ++dst_y) {
        const float source_y = (dst_y + 0.5f) * image.height / INPUT_SIZE - 0.5f;
        int y0 = static_cast<int>(std::floor(source_y));
        float fy = source_y - y0;
        if (y0 < 0) { y0 = 0; fy = 0.0f; }
        int y1 = y0 + 1 < image.height ? y0 + 1 : y0;
        for (int dst_x = 0; dst_x < INPUT_SIZE; ++dst_x) {
            const float source_x = (dst_x + 0.5f) * image.width / INPUT_SIZE - 0.5f;
            int x0 = static_cast<int>(std::floor(source_x));
            float fx = source_x - x0;
            if (x0 < 0) { x0 = 0; fx = 0.0f; }
            int x1 = x0 + 1 < image.width ? x0 + 1 : x0;

            const unsigned char* p00 = image.pixels + (y0 * image.width + x0) * 3;
            const unsigned char* p01 = image.pixels + (y0 * image.width + x1) * 3;
            const unsigned char* p10 = image.pixels + (y1 * image.width + x0) * 3;
            const unsigned char* p11 = image.pixels + (y1 * image.width + x1) * 3;
            for (int channel = 0; channel < 3; ++channel) {
                const float top = p00[channel] + (p01[channel] - p00[channel]) * fx;
                const float bottom = p10[channel] + (p11[channel] - p10[channel]) * fx;
                const float value = top + (bottom - top) * fy;
                network_input[channel * plane + dst_y * INPUT_SIZE + dst_x] =
                    static_cast<data_t>(value * SCALE / 255.0f + 0.5f);
            }
        }
    }
}

int decode_yolo_outputs(const data_t* out0, const data_t* out1,
                        int image_width, int image_height,
                        Detection* detections, int max_detections)
{
    Detection* candidates = new Detection[MAX_CANDIDATES];
    int candidate_count = 0;
    decode_head(out0, 13, 0, image_width, image_height, candidates, &candidate_count);
    decode_head(out1, 26, 1, image_width, image_height, candidates, &candidate_count);
    std::sort(candidates, candidates + candidate_count, HigherScore());

    int kept_count = 0;
    for (int i = 0; i < candidate_count && kept_count < max_detections; ++i) {
        bool suppressed = false;
        for (int j = 0; j < kept_count; ++j) {
            if (detections[j].class_id == candidates[i].class_id &&
                intersection_over_union(detections[j], candidates[i]) > NMS_IOU_THRESHOLD) {
                suppressed = true;
                break;
            }
        }
        if (!suppressed) {
            detections[kept_count++] = candidates[i];
        }
    }
    delete[] candidates;
    return kept_count;
}

void draw_detections(RgbImage* image, const Detection* detections, int count)
{
    static const unsigned char palette[8][3] = {
        {255, 64, 64}, {32, 220, 120}, {64, 160, 255}, {255, 210, 32},
        {220, 80, 220}, {32, 220, 220}, {255, 128, 32}, {240, 240, 240}
    };
    const int thickness = std::max(2, (image->width + image->height) / 700);
    for (int i = 0; i < count; ++i) {
        const int left = static_cast<int>(detections[i].left);
        const int top = static_cast<int>(detections[i].top);
        const int right = static_cast<int>(detections[i].right);
        const int bottom = static_cast<int>(detections[i].bottom);
        const unsigned char* color = palette[detections[i].class_id % 8];
        for (int t = 0; t < thickness; ++t) {
            for (int x = left; x <= right; ++x) {
                set_pixel(image, x, top + t, color);
                set_pixel(image, x, bottom - t, color);
            }
            for (int y = top; y <= bottom; ++y) {
                set_pixel(image, left + t, y, color);
                set_pixel(image, right - t, y, color);
            }
        }
        const int label_width = detections[i].class_id >= 10 ? 11 : 7;
        static const unsigned char label_background[3] = {0, 0, 0};
        fill_rectangle(image, left, top, left + label_width, top + 8, label_background);
        draw_class_id(image, left + 2, top + 2, detections[i].class_id, color);
    }
}
