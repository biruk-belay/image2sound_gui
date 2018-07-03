#include <QDebug>
#include <time.h>

#include "image2sound.h"
#include "rgb_to_midi.h"

void send_signal_to_composer(int task_id);

void *func(void *arg)
{
    int next_column = image2sound::trig_pts.thread_1_trig;
    int next_rand_point, next_midi_point;
    int i = 0;
    bool is_vector_full = false;
    timespec t_running;

    qDebug() << "MIDI FILE: started midi thread" << endl;
    thread_arg *th_arg = (thread_arg *) arg;
    task_param *t_param = (task_param *) th_arg->task_parameter;
    image_size *img_size = th_arg->img_size;
    composer_buffer *comp_buffer = (composer_buffer *) th_arg->extra_params;
    int task_id = t_param->task_id;
    int period = t_param->period;

    pthread_mutex_lock(&image2sound::sync_mutex[task_id]);
    while(!image2sound::is_triggered[task_id])
        pthread_cond_wait(&image2sound::sync_cond[task_id], &image2sound::sync_mutex[task_id]);

    pthread_mutex_unlock(&image2sound::sync_mutex[task_id]);

     qDebug() << "MIDI FILE: condition fulfilled thread " << task_id << "started" << period << endl;
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
        i += 1;

        //when reached the last column go back to the first
        if(next_column == img_size->width) {
            next_column = 0;
            is_vector_full = true;
        }

        //if buffer is full go to start of buffer
        if(i == img_size->width)
            i = 0;

        //go to sleep
        send_signal_to_composer(task_id);
        check_deadline_miss(t_running);
        wait_next_activation(t_running, period);
    }
    return NULL;
}

void send_signal_to_composer(int task_id)
{
    switch (task_id) {

    case THREAD1 :
        if(!image2sound::is_triggered[COMPOSER_th_1])
            send_signal(COMPOSER_th_1);
        break;
    case  THREAD2 :
        if(!image2sound::is_triggered[COMPOSER_th_2])
            send_signal(COMPOSER_th_2);
        break;
    case  THREAD3 :
        if(!image2sound::is_triggered[COMPOSER_th_3])
            send_signal(COMPOSER_th_3);
        break;
    case  THREAD4 :
        if(!image2sound::is_triggered[COMPOSER_th_4])
            send_signal(COMPOSER_th_4);
        break;
    }
}

