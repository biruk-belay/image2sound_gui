#include "image2sound.h"
#include <QApplication>

std::vector <float>  image2sound::rgb_vector;
std::vector <midi_data> image2sound::midi_vector;
composer_buffer image2sound::comp_buff[NUM_THREADS];

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    image2sound w;
    w.show();

    return a.exec();
}
