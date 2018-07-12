#include <QDebug>
#include <time.h>
#include "image2sound.h"
#include "rgb_to_midi.h"
#include "composer.h"


void *synthesizer(void *arg)
{
    int i = 0, dir = 1, resp;
    timespec t_running;
    thread_arg *th_arg = (thread_arg *) arg;
    task_param *t_param = (task_param *) th_arg->task_parameter;
    image_size *img_size = th_arg->img_size;
    int task_id = t_param->task_id;
    int period = t_param->period;
    synthesizer_data *synth = (synthesizer_data *) th_arg->extra_params;
    midi_address *midi_add = synth->midi_addr;
    int beat = synth->type;
    int *sequence = synth->sequence;

    int increment = 0;
    composer_buffer *midi_comp_buffer;

    qDebug() << "port" << midi_add->port << endl;

    snd_seq_t *seq = midi_add->seq_handler;
    int port = midi_add->port;
    int midi_channel = midi_add->channel;
    snd_seq_event_t ev;

    pthread_mutex_lock(&image2sound::sync_mutex[task_id]);
    while(!image2sound::is_triggered[task_id])
        pthread_cond_wait(&image2sound::sync_cond[task_id], &image2sound::sync_mutex[task_id]);

    pthread_mutex_unlock(&image2sound::sync_mutex[task_id]);
    qDebug() << "COMPOSER thread" <<task_id << "activated" << midi_channel << endl;

    switch (task_id) {

    case COMPOSER_th_1 :
        midi_comp_buffer = &image2sound::comp_buff[0];
        break;
    case COMPOSER_th_2 :
        midi_comp_buffer = &image2sound::comp_buff[1];
        break;
    case COMPOSER_th_3 :
        midi_comp_buffer = &image2sound::comp_buff[2];
        break;
    case COMPOSER_th_4 :
        midi_comp_buffer = &image2sound::comp_buff[3];
        break;
    }

    //change state to active
    image2sound::update_task_state(task_id);

    get_time(t_running);
    time_add_ms(&t_running, period);

    while(1) {
        snd_seq_ev_clear(&ev);
        snd_seq_ev_set_source(&ev, port);
        snd_seq_ev_set_subs(&ev);
        snd_seq_ev_set_direct(&ev);

        if(dir && *(sequence + increment)) {
            snd_seq_ev_set_noteon(&ev, midi_channel, midi_comp_buffer->buffer[i].note,
                                  midi_comp_buffer->buffer[i].volume);
           // qDebug() << "note" << (midi_comp_buffer->buffer[i].note) <<
             //          "volume " << midi_comp_buffer->buffer[i].volume << i <<endl;
        }
        else
            snd_seq_ev_set_noteoff(&ev, midi_channel, midi_comp_buffer->buffer[i].note,
                                   midi_comp_buffer->buffer[i].volume);



        if(dir) {
            i++;
            increment = (++increment) % beat ;
        }

         dir = (++dir) % 2;

        if(i == img_size->width)
            i = 0;

        resp = snd_seq_event_output(seq, &ev);

        if(resp < 0)
            qDebug() << "COMPOSER: thread" << task_id << "note is not sent" << resp << endl;
        else
            snd_seq_drain_output(seq);

        if(image2sound::cancel_th[task_id].kill_thread) {
            image2sound::tsk_state[task_id].is_active = false;
            image2sound::cancel_th[task_id].kill_thread = false;
            qDebug() << "killing thread in synthesizer" << task_id << endl;
            //clear vector
            pthread_exit(NULL);
        }

        else {
        check_deadline_miss(t_running, task_id);
        wait_next_activation(t_running, period);
        }
    }

    return NULL;
}

/* Read events from writeable port and route them to readable port 0  */
/* if NOTEON / OFF event with note < split_point. NOTEON / OFF events */
/* with note >= split_point are routed to readable port 1. All other  */
/* events are routed to both readable ports.                          */
void midi_route(snd_seq_t *seq_handle, int port)
{
    snd_seq_event_t *ev;
    do {
        snd_seq_event_input(seq_handle, &ev);
        //qDebug() << "received event " <<endl;
        //printf("recieved event \n");
        snd_seq_ev_set_subs(ev);
        snd_seq_ev_set_direct(ev);
        if ((ev->type == SND_SEQ_EVENT_NOTEON)||(ev->type == SND_SEQ_EVENT_NOTEOFF)) {
            const char *type = (ev->type == SND_SEQ_EVENT_NOTEON) ? "on " : "off";
            snd_seq_ev_set_source(ev, port);
            snd_seq_event_output_direct(seq_handle, ev);
            //qDebug() <<"MIDI ROUTE: recieved a note" << endl;
            //qDebug() <<"MIDI ROUTE:" << " note" << ev->data.note.note
                 //    << " vel" << ev->data.note.velocity <<endl;
        }
        //else {
        //qDebug() << "MIDI ROUTE: not a note" << endl;
        //}

        snd_seq_free_event(ev);
    }while (snd_seq_event_input_pending(seq_handle, 0) > 0);
}

void *alsa_handler(void *arg)
{
    thread_arg *th_arg = (thread_arg *) arg;
    midi_address *midi_add = (midi_address *) th_arg->extra_params;
    snd_seq_t *seq_handle = midi_add->seq_handler;
    int port = midi_add->port;
    task_param *t_param = th_arg->task_parameter;

    int task_id = t_param->task_id;
    int npfd;
    struct pollfd *pfd;

    npfd = snd_seq_poll_descriptors_count(seq_handle, POLLIN);
    pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
    snd_seq_poll_descriptors(seq_handle, pfd, npfd, POLLIN);

    //change state to active
    image2sound::update_task_state(task_id);

    while (1) {
        if (poll(pfd, npfd, 100000) > 0) {
          //  qDebug() << "event" << endl;
            midi_route(seq_handle, port);
        }
    }
}
