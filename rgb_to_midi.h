#ifndef RGB_TO_MIDI_H
#define RGB_TO_MIDI_H

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
