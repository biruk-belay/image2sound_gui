#include <QDebug>
#include <time.h>
#include "image2sound.h"
#include "rgb_to_midi.h"

#define get_time(t) clock_gettime(CLOCK_MONOTONIC, &t);
#define wait_next_activation(t, p) clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL); \
                              time_add_ms(&t, p);

#define send_signal(thread_id)    pthread_mutex_lock(&image2sound::sync_mutex[thread_id]); \
                                  image2sound::is_triggered[thread_id] = true; \
                                  pthread_cond_signal(&image2sound::sync_cond[thread_id]); \
                                  pthread_mutex_unlock(&image2sound::sync_mutex[thread_id]);

void send_signal_to_thread(int next_column);

#define MAX(x, y, z) x>y ? x>z ? x : z : y>z ? y : z
#define MIN(x, y, z) x<y ? x<z ? x : z : y<z ? y : z

int convert_to_hsb (unsigned char *rgb_input, float *hsb_output, image_size *img_size);
void process_rgb_to_midi(unsigned char *rgb_input, midi_data *output, image_size *img_size);
int convert_to_midi(float *hsb_input, midi_data *midi_output);
int convert_to_midi(float *hsb_input, midi_data *midi_output, image_size *img_size);

void *rgb_to_midi(void *arg)
{
    unsigned int i;
    static bool is_vector_full = false;
    unsigned int next_column = 0;
    struct timespec t_running;
    thread_arg *t_arg = (thread_arg *) arg;
    task_param *t_param = (task_param *) t_arg->task_parameter;

    int period = t_param->period;
    image_size *img_size = t_arg->img_size;
    unsigned int rgb_in_acolumn = img_size->height * 3;
    //a buffer to copy the current rgb column
    unsigned char rgb_buffer[rgb_in_acolumn];
    midi_data midi_buffer[img_size->height];

    qDebug() << "RGB TO MIDI: image size is" << img_size->height << " " << rgb_in_acolumn<< endl;
    //start clock
    get_time(t_running);
    time_add_ms(&t_running, period);

    while(1) {
        for(i = 0; i < rgb_in_acolumn; i++)
            rgb_buffer[i] = image2sound::rgb_vector[ i + (rgb_in_acolumn * next_column)];

        //call function to change rgb to midi
        process_rgb_to_midi(rgb_buffer, midi_buffer, img_size);

        if(!is_vector_full) {
            for(i = 0; i < img_size->height; i++)
                image2sound::midi_vector.push_back(midi_buffer[i]);
            }
        else
            for(i = 0; i < img_size->height; i++)
                 midi_buffer[i] = image2sound::midi_vector[i + (img_size->height * next_column)];

        next_column += 1;
        if(!image2sound::is_triggered[THREAD1] || !image2sound::is_triggered[THREAD2] ||
                !image2sound::is_triggered[THREAD3] || !image2sound::is_triggered[THREAD4]){
            send_signal_to_thread(next_column);
            qDebug() << "MIDI about to send a signal" << endl;
        }

        if(next_column == img_size->width) {
            next_column = 0;
            is_vector_full = true;
        }
        //qDebug() << "RGB TO MIDI: column is " << next_column << " " << img_size->width <<endl;
        //copy output midi to vector
        check_deadline_miss(t_running);
        wait_next_activation(t_running, period);
        //qDebug() << "RGB TO MIDI: woke up from sleep" << endl;
    }

    //it should never reach here
    return NULL;
}

void time_add_ms(struct timespec *t, int ms)
{
    t->tv_sec  += ms / 1000;
    t->tv_nsec += (ms % 1000) * 1000000;

    if(t->tv_nsec > 1000000000) {
        t->tv_sec += 1;
        t->tv_nsec -= 1000000000;
    }
}

void check_deadline_miss(struct timespec t1)
{
    struct timespec now;

    get_time(now);
    if(time_cmp(now, t1) > 0)
        qDebug() << "deadline miss" << endl;
}

int time_cmp(timespec t1, timespec t2)
{
    if (t1.tv_sec > t2.tv_sec) return 1;
    if (t1.tv_sec < t2.tv_sec) return -1;
    if (t1.tv_nsec > t2.tv_nsec) return 1;
    if (t1.tv_nsec < t2.tv_nsec) return -1;
    return 0;
}

void process_rgb_to_midi(unsigned char *rgb_input, midi_data *output, image_size *img_size)
{
    float hsb_buffer [img_size->height * 2];
    convert_to_hsb(rgb_input, hsb_buffer, img_size);
    convert_to_midi(hsb_buffer, output, img_size);
}

int convert_to_hsb (unsigned char *rgb_input, float *hsb_output, image_size *img_size)
{
    //qDebug() << "INSIDE TO HSB: entered hsb" <<endl;
    float min, max, delta;
    int i, j;
    int rgb_in_acolumn = img_size->height * 3;
    int red, blue, green;
    float r, g, b;
    float hue, brightness, sat;

    for(i = 0, j = 0; i < rgb_in_acolumn; i += 3, j += 2) {
        //copy the r,g and b values from the rgb buffer
        red   = *(rgb_input + i);
        green = *(rgb_input + i + 1);
        blue  = *(rgb_input + i + 2);

        assert((red <= 255)  || (red >= 0)   ||
              (green <= 255) || (green >= 0) ||
              (blue <= 255)  || (blue >= 0));

        r = (float) red/255.0;
        b = (float) blue/255.0;
        g = (float) green/255.0;

        min = MIN(r, b, g);
        max = MAX(r, b, g);

        brightness = max;
        delta = max - min;

        //if pixel is a shade of gray
        if(delta == 0 && max != 0){
            hue = -2;
            sat = max;       // s
            *(hsb_output + j) = hue;
            *(hsb_output +j + 1) = brightness * sat;
           // qDebug() << "INSIDE TO HSB: in if" <<endl;
            continue;
        }
        //if pixel is black
        else if(delta == 0 && max == 0) {
            sat = 1;
            brightness = 1;
            hue = -1;
            *(hsb_output + j) = hue;
            *(hsb_output +j + 1) = brightness * sat;
               // qDebug() << "INSIDE TO HSB: in else if" <<endl;
            continue;
        }

        else
            sat = delta / max;

        if(r == max)
            hue = ( g - b ) / delta;     // between yellow & magenta
        else if(g == max)
            hue = 2.0 + ( b - r ) / delta; // between cyan & yellow
        else if (b == max)
            hue = 4.0 + ( r - g ) / delta; // between magenta & cyan
        else
            hue = -2.0;
    //qDebug() << "INSIDE TO HSB: before writing " << i << " " << rgb_in_acolumn<< endl;
        *(hsb_output + j) = hue;
        *(hsb_output + j + 1) = brightness;
    }
    //qDebug() << "INSIDE TO HSB: done converting to hsb " << i << endl;
    return 0;
}

int convert_to_midi(float *hsb_input, midi_data *midi_output, image_size *img_size)
{
    unsigned int i;
    float hue, luminousity;

    for(i = 0; i < img_size->height; i += 2) {
        hue = *(hsb_input + i);
        luminousity = *(hsb_input + i + 1);

        if(hue == -1)
            (midi_output + i)->note = (unsigned char) MIDI_1;
        else if(hue == -2)
           (midi_output + i)->note = (unsigned char) MIDI_2;

        if(hue > -0.5 && hue <= 0.5)
           (midi_output + i)->note = (unsigned char) MIDI_3;

        else if(hue > 0.5 && hue <= 1)
           (midi_output + i)->note = (unsigned char) MIDI_4;

        else if(hue > 1.0 && hue <= 1.5)
            (midi_output + i)->note = (unsigned char) MIDI_5;

        else if(hue > 1.5 && hue <= 2.0)
            (midi_output + i)->note = (unsigned char) MIDI_6;

        else if(hue > 2.0 && hue <= 2.5)
            (midi_output + i)->note = (unsigned char) MIDI_7;

        else if(hue > 2.5 && hue <= 3.0)
            (midi_output + i)->note = (unsigned char) MIDI_8;

        else if(hue > 3.0 && hue <= 3.5)
            (midi_output + i)->note = (unsigned char) MIDI_9;

        else if(hue > 3.5 && hue <= 4.0)
            (midi_output + i)->note = (unsigned char) MIDI_10;

        else if(hue > 4.0 && hue <= 4.5)
            (midi_output + i)->note = (unsigned char) MIDI_11;

        else if(hue > 4.5 && hue <= 5.0)
            (midi_output + i)->note = (unsigned char) MIDI_12;

        else if(hue > 5.0 && hue <= 5.5)
            (midi_output + i)->note = (unsigned char) MIDI_13;
        else
            (midi_output + i)->note = (unsigned char) MIDI_14;

        (midi_output + i)->volume = luminousity * 127;
   // qDebug() << "INSIDE TO MIDI: midi note " << hue << " "<< (midi_output + i)->note << " vol "
           //   << (midi_output + i)->volume <<" " << luminousity *127 << endl;
        if((midi_output + i)->note == 0)
           exit(0);

    }
    return 1;

}

void send_signal_to_thread(int next_column)
{
    int thread_id;
    qDebug () << "getting inside signal" << endl;

    if(!image2sound::is_triggered[THREAD1] && next_column == image2sound::trig_pts.thread_1_trig) {
        thread_id = THREAD1;
        send_signal(thread_id);
    }

    if(!image2sound::is_triggered[THREAD2] && next_column == image2sound::trig_pts.thread_2_trig) {
        thread_id = THREAD2;
        send_signal(thread_id);
    }

    if(!image2sound::is_triggered[THREAD3] && next_column == image2sound::trig_pts.thread_3_trig) {
        thread_id = THREAD3;
        send_signal(thread_id);
    }

    if(!image2sound::is_triggered[THREAD4] && next_column == image2sound::trig_pts.thread_4_trig) {
        thread_id = THREAD4;
        send_signal(thread_id);
    }
}
