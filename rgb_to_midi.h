#ifndef RGB_TO_MIDI_H
#define RGB_TO_MIDI_H
#include <time.h>

#define get_time(t) clock_gettime(CLOCK_MONOTONIC, &t);
#define wait_next_activation(t, p) clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL); \
                              time_add_ms(&t, p);
void time_add_ms(struct timespec *t, int ms);
int time_cmp(struct timespec t1, struct timespec t2);
void check_deadline_miss(struct timespec t1);

enum MIDI {
    MIDI_1 = 64,      //80Hz
    MIDI_2,           //240HZ
    MIDI_3,
    MIDI_4,
    MIDI_5,
    MIDI_6,
    MIDI_7,
    MIDI_8,
    MIDI_9,
    MIDI_10,
    MIDI_11,
    MIDI_12,
    MIDI_13,
    MIDI_14,
};
void *rgb_to_midi(void *arg);
#endif // RGB_TO_MIDI_H
