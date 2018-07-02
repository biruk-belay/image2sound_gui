#include "image2sound.h"
#include <QApplication>

std::vector <float>  image2sound::rgb_vector;
std::vector <midi_data> image2sound::midi_vector;
composer_buffer image2sound::comp_buff[NUM_THREADS];
pthread_mutex_t image2sound::sync_mutex[NUM_THREADS];
pthread_cond_t  image2sound::sync_cond[NUM_THREADS];
bool image2sound::is_triggered[NUM_THREADS];
struct trigger_points image2sound::trig_pts;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    image2sound w;
    w.show();

    return a.exec();
}
