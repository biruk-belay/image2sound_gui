#include "image2sound.h"
#include <QApplication>

std::vector <unsigned  char>  image2sound::rgb_vector;
std::vector <midi_data> image2sound::midi_vector;
composer_buffer image2sound::comp_buff[NUM_THREADS];
pthread_mutex_t image2sound::sync_mutex[TOTAL_THREADS - 2];
pthread_cond_t  image2sound::sync_cond[TOTAL_THREADS - 2];
bool image2sound::is_triggered[TOTAL_THREADS - 2];
struct trigger_points image2sound::trig_pts [NUM_THREADS + 1] = {{90, 20},
                                                            {40, 200},
                                                            {40, 80},
                                                            {65, 49},
                                                            {0, 150}};
task_state image2sound::tsk_state[TOTAL_THREADS - 1];
rect_position image2sound::rect[NUM_THREADS + 1];
int image2sound::frame_width;
int image2sound::frame_height;
cancel_thread image2sound::cancel_th[TOTAL_THREADS];
rhythm image2sound::rhytm1[NUM_THREADS];
midi_address image2sound::midi_addr [NUM_THREADS + 1];
synthesizer_data image2sound::synth_data[NUM_THREADS + 1];
snd_seq_t *image2sound::seq_handler;

//int image2sound::thread_number[TOTAL_THREADS - 1];

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    image2sound w;

    w.show();

    return a.exec();
}
