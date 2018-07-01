#include <QString>
#include <vector>
#include <QDebug>
#include "image2sound.h"

#include "extract_rgb.h"
extern "C" {
#include "process_jpg.h"
}

void *extract_rgb(void *arg)
{
    /*Variables to process image*/
    image_size img_size;
    unsigned int pixel_buff_size;
    unsigned int i, k = 0;
    int j;

    image_info *img_info = (image_info *) arg;

    QString filename = *((QString *) img_info->filename);
    QByteArray ba = filename.toLatin1();

    char fname [ba.size() + 1];

    for(j = 0; j < ba.size() + 1; j++)
        fname [j] = ba.data()[j];
    fname[j] = '\0';

    qDebug() << "EXTRACT RGB: fname is" << fname << endl;

    get_image_size(fname, &img_size);
    *(img_info->img_size) = img_size;

    pixel_buff_size = img_size.width * img_size.height * PIXEL_COMPONENTS;
    unsigned char buffer [pixel_buff_size];
    unsigned char buffer_tx[pixel_buff_size];
    unsigned char *buff_ptr = buffer;

   extract_rgb_from_jpg(fname, buff_ptr);
    qDebug() << "EXTRAACT RGB: extracted the rgb, doing transpose " << endl;

    //Transpose the buffer
    for(i = 0; i < img_size.width * 3; i+=3)
        for(j = 0; j < img_size.height; j++) {
            buffer_tx[k] = buffer[i + img_size.width * 3 * j];
            buffer_tx[k + 1] = buffer[i + 1 + img_size.width * 3 * j];
            buffer_tx[k + 2] = buffer[i + 2 + img_size.width * 3 * j];
            k += 3;
        }

//  rgb_to_freq(buffer_tx, freq_mapping, img_size);
    image2sound::copy_to_vector(buffer_tx, pixel_buff_size);

    qDebug() << "EXTRAACT RGB: done extracting rgb" << endl;
    return NULL;
}
