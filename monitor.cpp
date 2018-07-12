#include <QDebug>
#include "rgb_to_midi.h"
#include "image2sound.h"

void *monitor(void *arg)
{
    QString task_name[] = {"Copy_1", "Copy_2", "Copy_3", "Copy_4",
                           "chan_1", "chan_2", "chan_3", "chan_4",
                           "alsa_handler", "rgb_to_midi", "monitor"};

    struct timespec t_running;
    thread_arg *t_arg = (thread_arg *) arg;
    task_param *t_param = t_arg->task_parameter;
    int task_id = t_param->task_id;
    int period  = t_param->period;
    int i;

    //change state to active
    image2sound::update_task_state(task_id);

    get_time(t_running);
    time_add_ms(&t_running, period);

    qDebug() << "MONITOR: size" <<sizeof(image2sound::tsk_state)/sizeof(task_state *) <<endl;
    while(1) {
        qDebug() << "Task id   " << "Missed deadlines " << endl;
        for(i = 0; i < (sizeof(image2sound::tsk_state)/sizeof(task_state *)); i++)
            if(image2sound::tsk_state[i].is_active)
                qDebug() << task_name[i] << image2sound::tsk_state[i].missed_deadline;

        qDebug() << endl;

        check_deadline_miss(t_running, task_id);
        wait_next_activation(t_running, period);
    }
}
