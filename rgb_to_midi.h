#ifndef RGB_TO_MIDI_H
#define RGB_TO_MIDI_H
#include <time.h>

#define get_time(t) clock_gettime(CLOCK_MONOTONIC, &t);
#define wait_next_activation(t, p) clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL); \
                              time_add_ms(&t, p);

#define send_signal(thread_id)    pthread_mutex_lock(&image2sound::sync_mutex[thread_id]); \
                                  image2sound::is_triggered[thread_id] = true; \
                                  pthread_cond_signal(&image2sound::sync_cond[thread_id]); \
                                  pthread_mutex_unlock(&image2sound::sync_mutex[thread_id]);

void time_add_ms(struct timespec *t, int ms);
int time_cmp(struct timespec t1, struct timespec t2);
void check_deadline_miss(struct timespec t1, int task_id);

#define  OCTAVE 12


enum key_chords{
    CMINOR,
    CMAJOR,
    DDORIAN,
};

typedef struct {
    key_chords key;
    int scale_1;
    int scale_2;
}keys;

enum MIDI {
    MIDI_1 = 71,
    MIDI_2,
    MIDI_3,
    MIDI_4,
    MIDI_5,
    MIDI_6,
    MIDI_7,
    MIDI_8,
    MIDI_9 ,
    MIDI_10,
    MIDI_11,
    MIDI_12,
    MIDI_13,
    MIDI_14,
};
void *rgb_to_midi(void *arg);
#endif // RGB_TO_MIDI_H
