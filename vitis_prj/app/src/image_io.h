#ifndef IMAGE_IO_H_INCLUDED
#define IMAGE_IO_H_INCLUDED

struct RgbImage {
    int width;
    int height;
    unsigned char* pixels;
};

bool load_image_from_sd(const char* filename, RgbImage* image);
bool save_jpeg_to_sd(const char* filename, const RgbImage& image, int quality);
void free_rgb_image(RgbImage* image);

#endif
