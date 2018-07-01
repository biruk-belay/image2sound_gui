#include <QDebug>
#include <time.h>
#include "image2sound.h"
#include "rgb_to_midi.h"

#define get_time(t) clock_gettime(CLOCK_MONOTONIC, &t);
#define wait_next_activation(t, p) clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL); \
                              time_add_ms(&t, p);

#define MAX(x, y, z) x>y ? x>z ? x : z : y>z ? y : z
#define MIN(x, y, z) x<y ? x<z ? x : z : y<z ? y : z

void time_add_ms(struct timespec *t, int ms);
int time_cmp(struct timespec t1, struct timespec t2);
void check_deadline_miss(struct timespec t1);
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
    std::vector<midi_data>::iterator it;

    it = image2sound::midi_vector.begin();
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
                //image2sound::midi_vector.insert(it, midi_buffer, midi_buffer + t_arg->img_size->height);
                image2sound::midi_vector.push_back(midi_buffer[i]);
            }
        else
            for(i = 0; i < img_size->height; i++)
                 midi_buffer[i] = image2sound::midi_vector[i + (img_size->height * next_column)];

        next_column += 1;

        if(next_column == img_size->width) {
            next_column = 0;
            is_vector_full = true;
        }
        //qDebug() << "RGB TO MIDI: column is " << next_column << " " << img_size->width <<endl;
        //copy output midi to vector
        check_deadline_miss(t_running);
        wait_next_activation(t_running, period);
        qDebug() << "RGB TO MIDI: woke up from sleep" << endl;
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
    //qDebug() << "INSIDE PROCESS RGB: entered process" <<endl;
    convert_to_hsb(rgb_input, hsb_buffer, img_size);
    //qDebug() << "INSIDE PROCESS RGB: done converting to hsb" <<endl;
    convert_to_midi(hsb_buffer, output, img_size);
    //qDebug() << "INSIDE PROCESS RGB: done converting to midi" <<endl;
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
        *(hsb_output + j + 1) = brightness * sat;
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

        (midi_output + i)->volume = (unsigned char) luminousity * 127;
    // qDebug() << "INSIDE TO MIDI: midi note " << hue << " "<< (midi_output + i)->note << " vol "
      //        << (midi_output + i)->volume <<" " << luminousity << endl;
    }
    return 1;

}
