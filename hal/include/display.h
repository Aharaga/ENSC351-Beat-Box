#ifndef DISPLAY_H
#define DISPLAY_H

// Initialize the DJ mixer display
int display_init(void);

// Show track information for both channels
void display_update(const char *trackA, const char *trackB,
                    int volumeA, int volumeB,
                    int playingA, int playingB,
                    int crossfade);

// Close the DJ mixer display
void display_close(void);

#endif