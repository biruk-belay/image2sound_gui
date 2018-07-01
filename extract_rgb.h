#ifndef EXTRACT_RGB_H
#define EXTRACT_RGB_H

#ifndef _IMG_SIZE
#define _IMG_SIZE 1
typedef struct{
    unsigned int width;
    unsigned int height;
}image_size;
#endif

void *extract_rgb(void *file);
#endif // EXTRACT_RGB_H
