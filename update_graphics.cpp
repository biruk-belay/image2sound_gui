#include <QDebug>

#include "image2sound.h"
#include "rgb_to_midi.h"

void *update_graphics(void *arg)
{
    struct timespec t_running;
    thread_arg *t_arg = (thread_arg *) arg;
    task_param *t_param = t_arg->task_parameter;
    int period = t_param->period;
    int task_id = t_param->task_id;
    int x = 20, y = 0;

    qDebug() << "started update graphics" << endl;
    get_time(t_running);
    time_add_ms(&t_running, period);

    while(1) {

        //image2sound::scene->addPixmap(image2sound::image.scaled(QSize((int)scene->width(),
          //                                                            (int)scene->height())));
        //scene->addRect(20, 10, 30, 10, QPen(QBrush(Qt::yellow), 2));
        x += 5;
        y += 5;

        qDebug() << "x is " << x << "y is " << y <<endl;

        check_deadline_miss(t_running, task_id);
        wait_next_activation(t_running, period);
    }
}
