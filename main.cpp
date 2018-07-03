#include "image2sound.h"
#include <QApplication>

std::vector <unsigned  char>  image2sound::rgb_vector;
std::vector <midi_data> image2sound::midi_vector;
composer_buffer image2sound::comp_buff[NUM_THREADS];
pthread_mutex_t image2sound::sync_mutex[TOTAL_THREADS - 2];
pthread_cond_t  image2sound::sync_cond[TOTAL_THREADS - 2];
bool image2sound::is_triggered[TOTAL_THREADS - 2];
struct trigger_points image2sound::trig_pts;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    image2sound w;
    w.show();

    return a.exec();
}
