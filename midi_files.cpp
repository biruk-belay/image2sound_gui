#include <QDebug>
#include <time.h>

#include "image2sound.h"
#include "rgb_to_midi.h"

void *func(void *arg)
{
    int next_column = image2sound::trig_pts.thread_1_trig;
    int next_rand_point, next_midi_point;
    int i;
    bool is_vector_full = false;
    timespec t_running;

    qDebug() << "started midi thread" << endl;
    thread_arg *th_arg = (thread_arg *) arg;
    task_param *t_param = (task_param *) th_arg->task_parameter;
    image_size *img_size = th_arg->img_size;
    composer_buffer *comp_buffer = (composer_buffer *) th_arg->extra_params;

    int task_id = t_param->task_id;
    int period = t_param->period;

    qDebug() << "thread id is " << task_id << endl;
    pthread_mutex_lock(&image2sound::sync_mutex[task_id]);
    while(!image2sound::is_triggered[task_id]) {
        qDebug() << "before being blocked" << endl;
        pthread_cond_wait(&image2sound::sync_cond[task_id], &image2sound::sync_mutex[task_id]);
    }
    pthread_mutex_unlock(&image2sound::sync_mutex[task_id]);

     qDebug() << "condition fulfilled " << endl;
    // start timer
    get_time(t_running);
    time_add_ms(&t_running, period);
    while(1) {
        //Pick a random pixel from the next column
        next_rand_point = rand() % img_size->height;
        next_midi_point = next_rand_point + img_size->height * next_column;

        //use mutex here
        if(!is_vector_full)
            comp_buffer->buffer.push_back(image2sound::midi_vector[next_midi_point]);
        else
            comp_buffer->buffer[i] = image2sound::midi_vector[next_midi_point];

        next_column += 1;
        if(next_column == img_size->width) {
            next_column = 0;
            is_vector_full = true;
            i = 0;
        }

        //todo
        //Fix why some midi_vectors are reading 0
         qDebug() << "MIDI THREAD: midi vector is " << image2sound::midi_vector[next_midi_point].note << endl;
         qDebug() << "MIDI THREAD:  midi point" << next_midi_point << " rand point " << next_rand_point <<
                     "next column " << next_column << endl;
        //go to sleep
         check_deadline_miss(t_running);
         wait_next_activation(t_running, period);
    }

    return NULL;
}
