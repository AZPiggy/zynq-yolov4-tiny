#include "image_io.h"

#include "ff.h"
#include "xil_printf.h"

#include <new>

#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_THREAD_LOCALS
#define STBI_MAX_DIMENSIONS 4096
#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"

namespace {

const unsigned int MAX_JPEG_FILE_SIZE = 16U * 1024U * 1024U;
const unsigned int MAX_IMAGE_PIXELS = 12U * 1024U * 1024U;

struct JpegWriteContext {
    FIL* file;
    bool ok;
};

void jpeg_write_callback(void* context, void* data, int size)
{
    JpegWriteContext* writer = static_cast<JpegWriteContext*>(context);
    UINT bytes_written = 0;
    if (!writer->ok || f_write(writer->file, data, static_cast<UINT>(size), &bytes_written) != FR_OK ||
        bytes_written != static_cast<UINT>(size)) {
        writer->ok = false;
    }
}

} // namespace

bool load_image_from_sd(const char* filename, RgbImage* image)
{
    image->width = 0;
    image->height = 0;
    image->pixels = 0;

    FIL file;
    FRESULT result = f_open(&file, filename, FA_READ);
    if (result != FR_OK) {
        xil_printf("ERROR: cannot open %s (%d)\r\n", filename, result);
        return false;
    }

    const unsigned int file_size = static_cast<unsigned int>(f_size(&file));
    if (file_size == 0 || file_size > MAX_JPEG_FILE_SIZE) {
        xil_printf("ERROR: image file size is invalid or exceeds 16 MB\r\n");
        f_close(&file);
        return false;
    }

    unsigned char* encoded = new (std::nothrow) unsigned char[file_size];
    if (encoded == 0) {
        xil_printf("ERROR: no memory for encoded image\r\n");
        f_close(&file);
        return false;
    }

    UINT bytes_read = 0;
    result = f_read(&file, encoded, file_size, &bytes_read);
    f_close(&file);
    if (result != FR_OK || bytes_read != file_size) {
        xil_printf("ERROR: failed to read complete image (%d/%u bytes)\r\n", bytes_read, file_size);
        delete[] encoded;
        return false;
    }

    int channels = 0;
    if (!stbi_info_from_memory(encoded, static_cast<int>(file_size), &image->width, &image->height, &channels) ||
        image->width <= 0 || image->height <= 0 ||
        static_cast<unsigned int>(image->width) * static_cast<unsigned int>(image->height) > MAX_IMAGE_PIXELS) {
        xil_printf("ERROR: invalid JPEG/PNG or decoded image exceeds 12 megapixels\r\n");
        delete[] encoded;
        image->width = image->height = 0;
        return false;
    }

    stbi_set_flip_vertically_on_load(0);
    image->pixels = stbi_load_from_memory(encoded, static_cast<int>(file_size),
                                          &image->width, &image->height, &channels, 3);
    delete[] encoded;
    if (image->pixels == 0) {
        xil_printf("ERROR: JPEG/PNG decode failed: %s\r\n", stbi_failure_reason());
        image->width = image->height = 0;
        return false;
    }

    xil_printf("Image loaded: %s (%dx%d)\r\n", filename, image->width, image->height);
    return true;
}

bool save_jpeg_to_sd(const char* filename, const RgbImage& image, int quality)
{
    FIL file;
    FRESULT result = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (result != FR_OK) {
        xil_printf("ERROR: cannot create %s (%d)\r\n", filename, result);
        return false;
    }

    stbi_flip_vertically_on_write(0);
    JpegWriteContext writer = {&file, true};
    const int encoded = stbi_write_jpg_to_func(jpeg_write_callback, &writer, image.width,
                                                image.height, 3, image.pixels, quality);
    result = f_close(&file);
    if (!encoded || !writer.ok || result != FR_OK) {
        xil_printf("ERROR: JPEG encode/write failed\r\n");
        return false;
    }

    xil_printf("Result saved: %s\r\n", filename);
    return true;
}

void free_rgb_image(RgbImage* image)
{
    if (image->pixels != 0) {
        stbi_image_free(image->pixels);
    }
    image->pixels = 0;
    image->width = image->height = 0;
}
