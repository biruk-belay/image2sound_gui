#ifndef IMAGE2SOUND_H
#define IMAGE2SOUND_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <pthread.h>
#include <vector>
#include <semaphore.h>
#include <QTimer>
#include <alsa/asoundlib.h>

#include "extract_rgb.h"
#include "monitor.h"

#define NUM_THREADS 4
#define NUM_CHANNELS 4
#define TOTAL_THREADS 13
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
    MONITOR_THREAD,
    UPDATE_SCENE,
    EXTRACT_RGB,
};

enum tempo {
   QUARTER  = 8,
   EIGTH    = 16,
   SIXTINTH = 32,
};

typedef struct {
    bool kill_thread;
}cancel_thread;

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
    int missed_deadline;
    bool is_active;
}task_state;

typedef struct {
    task_param *task_parameter;
    image_size *img_size;
    void *extra_params;
}thread_arg;

typedef struct {
    int note;
    int volume;
}midi_data;

typedef struct {
    std::vector<midi_data> buffer;
}composer_buffer;

typedef struct {
    snd_seq_t *seq_handler;
    int port;
    int channel;
}midi_address;

typedef struct {
    midi_address *midi_addr;
    enum tempo type;
    int *sequence;
}synthesizer_data;

typedef struct {
    int x;
    int y;
}rect_position;

typedef struct {
    int quarter[QUARTER];
    int eigth{EIGTH};
    int sixtinth[SIXTINTH];
}rhythm;

struct trigger_points {
    int x;
    int y;
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

    //variables and methods used to set task properties
    pthread_t threads [TOTAL_THREADS];
    pthread_attr_t attr[TOTAL_THREADS];
    struct sched_param sch_par;
    thread_arg th_args[TOTAL_THREADS];
    task_param tp[TOTAL_THREADS];
    void init_task_param();


    //vectors to exchange data between tasks
    static std::vector <unsigned char> rgb_vector;
    static std::vector<midi_data> midi_vector;
    static composer_buffer comp_buff[NUM_THREADS];
    static void copy_to_vector(unsigned char*, unsigned int size);


    //variables to monitor task states
    static bool is_triggered[TOTAL_THREADS - 2];
    static struct trigger_points trig_pts[NUM_THREADS + 1];
    static task_state tsk_state[TOTAL_THREADS - 1];
    void init_task_state();
    static void update_task_state(int task_id);
    static void update_missed_deadline(int task_id);
    static cancel_thread cancel_th[TOTAL_THREADS];

    //synchronzation variables and methods
    static pthread_mutex_t sync_mutex[TOTAL_THREADS - 2];
    static pthread_cond_t  sync_cond[TOTAL_THREADS  - 2];
    void init_mux();
    void init_cond();

    //Variables for graphics
    QString imagePath;
    QGraphicsScene *scene;
    QPixmap image;
    QTimer *mTimer;
    static rect_position rect[NUM_THREADS + 1];
    static int frame_width, frame_height;

    //Variables used by ALSA
    snd_seq_t *seq_handler;
    int alsa_output_port[NUM_THREADS + 1];
    int alsa_input_port;
    static midi_address midi_addr [NUM_THREADS + 1];
    static synthesizer_data synth_data[NUM_THREADS + 1];
    static rhythm rhytm1[NUM_THREADS];
    static void generate_rhythm(int *rhythm, enum tempo type);



private:
    Ui::image2sound *ui;
    int open_seq(snd_seq_t **seq_handle, int *in_ports, int *out_ports);

private slots:
    void load_button_pressed();
    void play_button_pressed();
    void stop_button_pressed();
    void update_scene();
};

#endif // IMAGE2SOUND_H
