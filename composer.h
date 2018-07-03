#ifndef COMPOSER_H
#define COMPOSER_H
#include <alsa/asoundlib.h>

void *composer(void *arg);
void *alsa_handler(void *arg);
#endif // COMPOSER_H
