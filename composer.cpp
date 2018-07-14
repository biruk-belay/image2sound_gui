#include <QDebug>
#include <time.h>
#include "image2sound.h"
#include "rgb_to_midi.h"
#include "composer.h"


#define filter_bass(ptr) midi[0].note = 45; \
                        for(k = ptr; k < ptr + 5; k++) { \
                            if(midi_comp_buffer->buffer[k].note > midi[0].note) \
                                 midi[0].note = midi_comp_buffer->buffer[k].note; \
                            if(midi[0].note > 65) \
                                midi[0].note = 65;  } \
                            if(midi_comp_buffer->buffer[k].volume < 90) \
                                    midi[0].volume = 100;
                        //qDebug() << "notes in bass"<< midi[0].note \
                          //       << midi[0].volume << ptr << endl;


#define filter_others(ptr) for(k = 0; k < 3; k++, n++) { \
                                    n = ptr + (rand() % 5); \
                                    midi[k].note = midi_comp_buffer->buffer[n].note; \
                                    midi[k].volume = midi_comp_buffer->buffer[n].volume;}
                                        //qDebug() << "notes in others "<< midi[k].note \
                                        //<< midi[k].volume << ptr << n << endl; }

#define send_on_note(channel, notes) for(k = 0; k < notes; k++) { \
                                        snd_seq_ev_set_noteon(&ev, channel, \
                                        midi[k].note, \
                                        midi[k].volume); \
                                        resp = snd_seq_event_output_direct(seq, &ev); }
                                       // if(resp < 0) \
                                            qDebug() << "COMPOSER: thread" << task_id \
                                                 << "note is not sent" << resp << endl; \
                                        else qDebug() << "not a note"<< endl; }


#define send_off_note(channel, notes) for(k = 0; k < notes; k++) { \
                                        snd_seq_ev_set_noteoff(&ev, channel, \
                                        midi[k].note, \
                                        midi[k].volume); \
                                        resp = snd_seq_event_output_direct(seq, &ev); }
                                        //if(resp < 0) \
                                              qDebug() << "COMPOSER: thread" << task_id \
                                                << "note is not sent" << resp << endl;  \
                                        else qDebug() << "not a note"<< endl; }


void *synthesizer(void *arg)
{
    int resp;
    int ptr = 0, on = 1, k, n;
    timespec t_running;

    thread_arg *th_arg = (thread_arg *) arg;
    task_param *t_param = (task_param *) th_arg->task_parameter;
    image_size *img_size = th_arg->img_size;

    int task_id = t_param->task_id;
    int period = t_param->period;

    synthesizer_data *synth = (synthesizer_data *) th_arg->extra_params;
    midi_address *midi_add = synth->midi_addr;
    snd_seq_t *seq = midi_add->seq_handler;
    int midi_channel = midi_add->channel;
    int port = midi_add->port;

    int beat = synth->type;
    int *sequence = synth->sequence;
    enum instrument instr = synth->instr;
    int increment = 0;

    midi_data midi[3];
    composer_buffer *midi_comp_buffer;
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

        switch(instr) {

        case BASS:
             //qDebug() << "bass " << task_id;
            if(on && *(sequence + increment)) {
                filter_bass(ptr);
                send_on_note(midi_channel, 1);
            }
            else
                send_off_note(midi_channel, 1);
            break;

        case PIANO:
            //qDebug() << "piano " << task_id;
            if(on && *(sequence + increment)) {
                filter_others(ptr);
                send_on_note(midi_channel, 3);
            }
            else
                send_off_note(midi_channel, 3);

            if(ptr == img_size->width / 2) {
                image2sound::generate_rhythm(sequence, EIGTH, PIANO);
                qDebug() << "changed rhythm" << endl;
            }
            break;

        case GUITAR :
            //qDebug() << "piano " << task_id;
            if(on && *(sequence + increment)) {
                filter_others(ptr);
                send_on_note(midi_channel, 3);
            }
            else
                send_off_note(midi_channel, 3);
            break;

        }
/*

    if(on && *(sequence + increment)) {
            snd_seq_ev_set_noteon(&ev, midi_channel, midi_comp_buffer->buffer[ptr].note,
                                  midi_comp_buffer->buffer[ptr].volume);
            qDebug() << "note" << (midi_comp_buffer->buffer[ptr].note) <<
                       "volume " << midi_comp_buffer->buffer[ptr].volume << ptr << task_id << endl;
        }
        else
            snd_seq_ev_set_noteoff(&ev, midi_channel, midi_comp_buffer->buffer[ptr].note,
                                   midi_comp_buffer->buffer[ptr].volume);

    resp = snd_seq_event_output_direct(seq, &ev);
    //snd_seq_drain_output(seq);

    qDebug() <<"in event" << ev.data.note.note << ev.data.note.velocity << task_id << endl;
    if(resp < 0)
        qDebug() << "COMPOSER: thread" << task_id
            << "note is not sent" << resp << endl;
    //else snd_seq_drain_output(seq);
*/
    on += 1;
    on %= 2;

    if(on) {
        ptr += 1;
        increment = (++increment) % beat ;
    }


        if(ptr == img_size->width)
            ptr = 0;


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
        snd_seq_ev_set_source(ev, port);
        snd_seq_ev_set_subs(ev);
        snd_seq_ev_set_direct(ev);
        if ((ev->type == SND_SEQ_EVENT_NOTEON)||(ev->type == SND_SEQ_EVENT_NOTEOFF)) {
            const char *type = (ev->type == SND_SEQ_EVENT_NOTEON) ? "on " : "off";

            snd_seq_event_output_direct(seq_handle, ev);
            //snd_seq_drain_output(seq_handle);
            //qDebug() <<"MIDI ROUTE: recieved a note" << endl;
            //qDebug() <<"MIDI ROUTE:" << " note" << ev->data.note.note
                 //    << " vel" << ev->data.note.velocity <<endl;
        }
        //else {
        //qDebug() << "MIDI ROUTE: not a note" << endl;
        //}

        //snd_seq_free_event(ev);
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
        //if (poll(pfd, npfd, 100000) > 0) {
          //  qDebug() << "event" << endl;
            midi_route(seq_handle, port);
        //}
    }
}
