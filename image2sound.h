#ifndef IMAGE2SOUND_H
#define IMAGE2SOUND_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <pthread.h>
#include <vector>
#include <semaphore.h>

#include "extract_rgb.h"

#define NUM_THREADS 4
#define fill_task_param(threadId, arg, wcet, period, deadline, priority) \
                    tp[threadId] = {arg, wcet, period, deadline, priority, 0, 0, 0, 0};

#define    EXTRACT_RGB  0
#define    RGB_TO_MIDI  1

#define    THREAD_1     0
#define    THREAD_2     1
#define    THREAD_3     2
#define    THREAD_4     3
#define    COMPOSER     4

typedef struct
{
    void *arg;
    long wcet;
    int period;
    int deadline;  //relative deadline
    int priority;
    struct timespec activation_t;
    struct timespec dead_line;
}task_param;

typedef struct {
    void *task_parameter;
    image_size *img_size;
}thread_arg;

typedef struct {
    unsigned char note;
    unsigned char volume;
}midi_data;

typedef struct {
    QString *filename;
    image_size *img_size;
}image_info;

typedef struct {
    std::vector<midi_data> buffer;
}composer_buffer;

struct trigger_points {
    int thread_1_trig;
    int thread_2_trig;
    int thread_3_trig;
    int thread_4_trig;
};

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
    task_param tp[NUM_THREADS + 2];
    pthread_t threads [NUM_THREADS];
    pthread_attr_t attr[NUM_THREADS];
    struct trigger_points trig_pts;

    struct sched_param sch_par;
    static std::vector <float> rgb_vector;
    static std::vector<midi_data> midi_vector;
    static composer_buffer comp_buff[NUM_THREADS];

    QString imagePath;

    void init_task_param();
    static void copy_to_vector(unsigned char*, unsigned int size);

private:
    Ui::image2sound *ui;
    QPixmap image;
    QGraphicsScene *scene;

private slots:
    void load_button_pressed();
    void play_button_pressed();
    void stop_button_pressed();
};

#endif // IMAGE2SOUND_H
