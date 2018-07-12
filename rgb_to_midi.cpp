#include <QDebug>
#include <time.h>
#include "image2sound.h"
#include "rgb_to_midi.h"

void send_signal_to_thread(int next_column);

#define MAX(x, y, z) x>y ? x>z ? x : z : y>z ? y : z
#define MIN(x, y, z) x<y ? x<z ? x : z : y<z ? y : z

int c_minor[]  = {1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0};
int d_dorian[] = {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0};
int c_major[]  = {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1};

int red_mapping[14];
int blue_mapping[14];
int green_mapping[14];

int convert_to_hsb (unsigned char *rgb_input, float *hsb_output, image_size *img_size);
void process_rgb_to_midi(unsigned char *rgb_input, midi_data *output, image_size *img_size);
int convert_to_midi(float *hsb_input, midi_data *midi_output, image_size *img_size);
void generate_notes(int *notes, keys key);

int *red_notes, *blue_notes, *green_notes;
void *rgb_to_midi(void *arg)
{
    unsigned int i;
    bool is_vector_full = false;
    unsigned int next_column = 0;
    struct timespec t_running;
    thread_arg *t_arg = (thread_arg *) arg;
    task_param *t_param = (task_param *) t_arg->task_parameter;

    int period = t_param->period;
    int task_id = t_param->task_id;

    image_size *img_size = t_arg->img_size;
    unsigned int rgb_in_acolumn = img_size->height * 3;
    //a buffer to copy the current rgb column
    unsigned char rgb_buffer[rgb_in_acolumn];
    midi_data midi_buffer[img_size->height];
    keys key;

    qDebug() << "generate red notes" << endl;
    key = {DDORIAN, 2, 4};
    generate_notes(red_mapping, key);

    qDebug() << "generate green notes" << endl;
    key = {CMINOR, 5, 7};
    generate_notes(green_mapping, key);

    qDebug() << "generate blue notes" << endl;
    key = {CMAJOR, 3, 6};
    generate_notes(blue_mapping, key);

    //change state to active
    image2sound::update_task_state(task_id);

    qDebug() << "RGB TO MIDI: image size is" << img_size->height << " " << rgb_in_acolumn<< endl;
    //start clock
    get_time(t_running);
    time_add_ms(&t_running, period);


    while(1) {
        for(i = 0; i < rgb_in_acolumn; i++) {
            rgb_buffer[i] = image2sound::rgb_vector[ i + (rgb_in_acolumn * next_column)];
            //qDebug() << rgb_buffer[i] << endl;
        }

        //call function to change rgb to midi
        process_rgb_to_midi(rgb_buffer, midi_buffer, img_size);

        if(!is_vector_full) {
            for(i = 0; i < img_size->height; i++)
                image2sound::midi_vector.push_back(midi_buffer[i]);

        }
        else
            for(i = 0; i < img_size->height; i++) {
                 image2sound::midi_vector[i + (img_size->height * next_column)] = midi_buffer[i];
                // qDebug() << "note" << image2sound::midi_vector[i + (img_size->height * next_column)].note <<
                  //           is_vector_full << endl;
            }
        image2sound::rect[NUM_THREADS].x = next_column;
        image2sound::rect[NUM_THREADS].y = 150;

        next_column += 1;
        if(!image2sound::is_triggered[THREAD1] || !image2sound::is_triggered[THREAD2] ||
                !image2sound::is_triggered[THREAD3] || !image2sound::is_triggered[THREAD4]){
            send_signal_to_thread(next_column);
            //qDebug() << "MIDI about to send a signal" << endl;
        }

        if(next_column == img_size->width) {
            next_column = 0;
            is_vector_full = true;
            //pthread_exit(NULL);
        }
        //qDebug() << "RGB TO MIDI: column is " << next_column << " " << img_size->width <<endl;
        //copy output midi to vector

        //If signaled to be killed, clean up and exit
        if(image2sound::cancel_th[task_id].kill_thread) {
            image2sound::tsk_state[task_id].is_active = false;
            image2sound::cancel_th[task_id].kill_thread = false;

            qDebug() << "killing thread " << task_id <<endl;
            //clear vector
            pthread_exit(NULL);
        }

        //
        else {
        check_deadline_miss(t_running, task_id);
        wait_next_activation(t_running, period);
        //qDebug() << "RGB TO MIDI: woke up from sleep" << endl;

        }
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

void check_deadline_miss(struct timespec t1, int task_id)
{
    struct timespec now;

    get_time(now);
    if(time_cmp(now, t1) > 0) {
        qDebug() << "deadline miss" << endl;
        image2sound::update_missed_deadline(task_id);
    }
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
    int i;
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

        /*
        //if pixel is a shade of gray
        if(delta == 0 && max != 0){
            hue = -2;
            sat = max;       // s
            *(hsb_output + j) = hue;
            *(hsb_output +j + 1) = brightness * sat;
            continue;
        }
        //if pixel is black
        else if(delta == 0 && max == 0) {
            sat = 1;
            brightness = 1;
            hue = -1;
            *(hsb_output + j) = hue;
            *(hsb_output +j + 1) = brightness * sat;
            continue;
        }

        else
            sat = delta / max;
*/
        if(max > 0.0)
            sat = delta/max;
        else {
            sat = 0.0;
            hue = -1;
            *(hsb_output + j) = hue;
            *(hsb_output + j + 1) = brightness;
           // *(hsb_output + j + 2) = sat;
            return 0;
        }

        if(r == max)
            hue = ( g - b ) / delta;     // between yellow & magenta
        else if(g == max)
            hue = 2.0 + ( b - r ) / delta; // between cyan & yellow
        else //if (b == max)
            hue = 4.0 + ( r - g ) / delta; // between magenta & cyan
        //else
          //  hue = -2.0;
        *(hsb_output + j) = hue;
        *(hsb_output + j + 1) = brightness;
        //*(hsb_output + j + 2) = sat;
    }
    return 0;
}

int convert_to_midi(float *hsb_input, midi_data *midi_output, image_size *img_size)
{
    unsigned int i, j;
    unsigned char volume;
    float hue, luminousity;

    for(i = 0, j = 0; i < img_size->height; i++, j += 2) {
        hue = *(hsb_input + j);
        luminousity = *(hsb_input + j + 1);

        //black
        if(hue == -1)
            (midi_output + i)->note = MIDI_1;

        //shades of red
        else if(hue > -0.5 && hue <= -0.0713)
           (midi_output + i)->note =  red_mapping[0];

        else if(hue > -0.0713 && hue <= 0.0715)
            (midi_output + i)->note =  red_mapping[1];

        else if(hue > 0.0715 && hue <= 0.214)
            (midi_output + i)->note =  red_mapping[2];

        else if(hue > 0.214 && hue <= 0.357)
            (midi_output + i)->note =  red_mapping[3];

        else if(hue > 0.357 && hue <= 0.5)
            (midi_output + i)->note =  red_mapping[4];

        else if(hue > 0.5 && hue <= 0.6431)
            (midi_output + i)->note =  red_mapping[5];

        else if(hue > 0.6431 && hue <= 0.786)
            (midi_output + i)->note =  red_mapping[6];

        else if(hue > 0.786 && hue <= 0.9289)
            (midi_output + i)->note =  red_mapping[7];

        else if(hue > 0.9289 && hue <= 1.07)
            (midi_output + i)->note =  red_mapping[8];

        else if(hue > 1.07 && hue <= 1.214)
            (midi_output + i)->note =  red_mapping[9];

        else if(hue > 1.214 && hue <= 1.357)
            (midi_output + i)->note =  red_mapping[10];

        else if(hue > 1.357 && hue <= 1.5)
            (midi_output + i)->note =  red_mapping[11];

        else if(hue > 1.5 && hue <= 1.643)
            (midi_output + i)->note =  red_mapping[12];

        else if(hue > 1.643 && hue <= 2.07)
            (midi_output + i)->note =  red_mapping[13];

        else if(hue > 2.07 && hue <= 2.21)
            (midi_output + i)->note =  green_mapping[0];

        else if(hue > 2.21 && hue <= 2.357)
            (midi_output + i)->note =  green_mapping[1];

        else if(hue > 2.357 && hue <= 2.5)
            (midi_output + i)->note =  green_mapping[2];

        else if(hue > 2.5 && hue <= 2.643)
            (midi_output + i)->note =  green_mapping[3];

        else if(hue > 2.643 && hue <= 2.786)
            (midi_output + i)->note =  green_mapping[4];

        else if(hue > 2.786 && hue <= 2.929)
            (midi_output + i)->note =  green_mapping[5];

        else if(hue > 2.929 && hue <= 3.07)
            (midi_output + i)->note =  green_mapping[6];

        else if(hue > 3.07 && hue <= 3.214)
            (midi_output + i)->note =  green_mapping[7];

        else if(hue > 3.214 && hue <= 3.357)
            (midi_output + i)->note =  green_mapping[8];

        else if(hue > 3.357 && hue <= 3.5)
            (midi_output + i)->note =  green_mapping[9];

        else if(hue > 3.5 && hue <= 3.643)
            (midi_output + i)->note =  green_mapping[10];

        else if(hue > 3.643 && hue <= 3.786)
            (midi_output + i)->note =  green_mapping[11];

        else if(hue > 3.786 && hue <= 3.929)
            (midi_output + i)->note =  green_mapping[12];

        else if(hue > 3.929 && hue <= 4.07)
            (midi_output + i)->note =  green_mapping[13];

        else if(hue > 4.07 && hue <= 4.21)
            (midi_output + i)->note =  blue_mapping[0];

        else if(hue > 4.21 && hue <= 4.357)
            (midi_output + i)->note =  green_mapping[1];

        else if(hue > 4.357 && hue <= 4.5)
            (midi_output + i)->note =  green_mapping[2];

        else if(hue > 4.5 && hue <= 4.643)
            (midi_output + i)->note =  green_mapping[3];

        else if(hue > 4.643 && hue <= 4.786)
            (midi_output + i)->note =  green_mapping[4];

        else if(hue > 4.786 && hue <= 4.929)
            (midi_output + i)->note =  green_mapping[5];

        else if(hue > 4.929 && hue <= 5.07)
            (midi_output + i)->note =  green_mapping[6];

        else if(hue > 5.07 && hue <= 5.21)
            (midi_output + i)->note =  green_mapping[7];

        else if(hue > 5.21 && hue <= 5.357)
            (midi_output + i)->note =  green_mapping[8];

        else if(hue > 5.357 && hue <= 5.5)
            (midi_output + i)->note =  green_mapping[9];

        else if(hue > 5.5 && hue <= 5.643)
            (midi_output + i)->note =  green_mapping[10];

        else if(hue > 5.643 && hue <= 5.786)
            (midi_output + i)->note =  green_mapping[11];

        else if(hue > 5.786 && hue <= 5.929)
            (midi_output + i)->note =  green_mapping[12];

        else if(hue > 5.929 && hue <= 6.07)
            (midi_output + i)->note =  green_mapping[13];

        else
            (midi_output + i)->note = red_mapping[0];

        volume = luminousity * 127;
        if(volume < 60)
            volume = 60;

        (midi_output + i)->volume = volume;

        //qDebug() << "note " << (midi_output + i)->note << "volume " << (midi_output + i)->volume <<endl;
    }
    return 1;

}

void send_signal_to_thread(int next_column)
{
    int thread_id;

    if(!image2sound::is_triggered[THREAD1] && next_column == image2sound::trig_pts[THREAD1].x) {
        thread_id = THREAD1;
        send_signal(thread_id);
    }

    if(!image2sound::is_triggered[THREAD2] && next_column == image2sound::trig_pts[THREAD2].x) {
        thread_id = THREAD2;
        send_signal(thread_id);
    }

    if(!image2sound::is_triggered[THREAD3] && next_column == image2sound::trig_pts[THREAD3].x) {
        thread_id = THREAD3;
        send_signal(thread_id);
    }

    if(!image2sound::is_triggered[THREAD4] && next_column == image2sound::trig_pts[THREAD4].x) {
        thread_id = THREAD4;
        send_signal(thread_id);
    }
}

void generate_notes(int *notes, keys key)
{
    int i, j = 0;
    int *chord_ptr;
    int scale;

    switch (key.key) {

    case CMINOR :
        chord_ptr = c_minor;
        break;

    case DDORIAN :
        chord_ptr = d_dorian;
        break;

    case CMAJOR :
        chord_ptr = c_major;
        break;

    default :
        chord_ptr = c_minor;
    }

    for(i = 0; i < OCTAVE * 2; i++) {
        if(i < OCTAVE)
            scale = key.scale_1;
        else
            scale = key.scale_2;

        if(*(chord_ptr + (i % OCTAVE)) == 0)
            continue;
        else {
            *(notes + j)= scale * OCTAVE + (i % OCTAVE);
            qDebug() << *(notes + j);
            j++;
        }
    }
}
