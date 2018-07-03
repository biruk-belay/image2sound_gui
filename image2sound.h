#ifndef IMAGE2SOUND_H
#define IMAGE2SOUND_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <pthread.h>
#include <vector>
#include <semaphore.h>
#include <alsa/asoundlib.h>

#include "extract_rgb.h"


#define NUM_THREADS 4
#define TOTAL_THREADS 11
#define ALSA_INPUT_PORTS 1
#define ALSA_OUTPUT_PORTS 5

#define fill_task_param(threadId, id, arg, wcet, period, deadline, priority) \
                    tp[threadId] = {id, arg, wcet, period, deadline, priority, 0, 0, 0, 0};
enum Thread_id {
    THREAD1 = 0,
    THREAD2,
    THREAD3,
    THREAD4,
    COMPOSER_th_1,
    COMPOSER_th_2,
    COMPOSER_th_3,
    COMPOSER_th_4,
    ALSA_THREAD,
    RGB_TO_MIDI,
    EXTRACT_RGB,
};

typedef struct
{
    int task_id;
    void *arg;
    long wcet;
    int period;
    int deadline;  //relative deadline
    int priority;
    struct timespec activation_t;
    struct timespec dead_line;
}task_param;

typedef struct {
    task_param *task_parameter;
    image_size *img_size;
    void *extra_params;
}thread_arg;

typedef struct {
    unsigned char note;
    unsigned char volume;
}midi_data;

typedef struct {
    std::vector<midi_data> buffer;
}composer_buffer;

struct trigger_points {
    int thread_1_trig;
    int thread_2_trig;
    int thread_3_trig;
    int thread_4_trig;
};

typedef struct {
    snd_seq_t *seq_handler;
    int port;
}midi_address;

namespace Ui {
class image2sound;
}

class image2sound : public QMainWindow
{
    Q_OBJECT

public:
    explicit image2sound(QWidget *parent = 0);
    ~image2sound();
    image_size img_size_global;
    task_param tp[TOTAL_THREADS];
    pthread_t threads [TOTAL_THREADS];
    pthread_attr_t attr[TOTAL_THREADS];
    thread_arg th_args[TOTAL_THREADS];

    struct sched_param sch_par;
    static std::vector <unsigned char> rgb_vector;
    static std::vector<midi_data> midi_vector;

    static composer_buffer comp_buff[NUM_THREADS];
    static pthread_mutex_t sync_mutex[TOTAL_THREADS - 2];
    static pthread_cond_t  sync_cond[TOTAL_THREADS - 2];
    static bool is_triggered[TOTAL_THREADS - 2];
    static struct trigger_points trig_pts;

    QString imagePath;

    void init_task_param();
    static void copy_to_vector(unsigned char*, unsigned int size);
    void init_mux();
    void init_cond();

    //Variables for alsa
    snd_seq_t *seq_handler;
    int alsa_output_port[NUM_THREADS + 1];
    int alsa_input_port;
    midi_address midi_addr;

private:
    Ui::image2sound *ui;
    QPixmap image;
    QGraphicsScene *scene;
    int open_seq(snd_seq_t **seq_handle, int *in_ports, int *out_ports);

private slots:
    void load_button_pressed();
    void play_button_pressed();
    void stop_button_pressed();
};

#endif // IMAGE2SOUND_H
