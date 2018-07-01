#ifndef PROCESS_JPG_H
#define PROCESS_JPG_H
#include "extract_rgb.h"

#define PIXEL_COMPONENTS 3
int get_image_size(char *filename, image_size *size);
int extract_rgb_from_jpg(char *filename, unsigned char *buffer);
#endif // PROCESS_JPG_H
