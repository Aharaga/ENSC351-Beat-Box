#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

bool Audio_init(const char *trackA, const char *trackB);

void Audio_cleanup(void);

void Audio_setVolumeA(float vol);
void Audio_setVolumeB(float vol);

float Audio_getVolumeA(void);
float Audio_getVolumeB(void);

//Cross fade
void Audio_setCrossfade(float mix);
float Audio_getCrossfade(void);

int playSingleWAV(const char *filename);

#endif