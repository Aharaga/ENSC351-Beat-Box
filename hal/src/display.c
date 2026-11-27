#include "display.h"
#include "oled.h"
#include <stdio.h>

// Initialize the DJ mixer display
int display_init(void)
{
    return oled_init();
}

// Update track information, volume, and mix
void display_update(const char *trackA, const char *trackB,
                    int volumeA, int volumeB,
                    int playingA, int playingB,
                    int crossfade)
{
    char line[22];

    // Keep values between 0 and 100
    if (volumeA < 0) volumeA = 0;
    if (volumeA > 100) volumeA = 100;
    if (volumeB < 0) volumeB = 0;
    if (volumeB > 100) volumeB = 100;
    if (crossfade < 0) crossfade = 0;
    if (crossfade > 100) crossfade = 100;

    // Calculate how much of each track is mixed
    int mixA = 100 - crossfade;
    int mixB = crossfade;

    oled_clear();

    // Display title
    oled_print_at(0, "DJ MIXER");

    // Channel A
    snprintf(line, sizeof(line), "A: %.17s",
             trackA ? trackA : "");
    oled_print_at(2, line);

    snprintf(line, sizeof(line), "VOL %d%% %s",
             volumeA, playingA ? "PLAY" : "PAUSE");
    oled_print_at(3, line);

    // Channel B
    snprintf(line, sizeof(line), "B: %.17s",
             trackB ? trackB : "");
    oled_print_at(5, line);

    snprintf(line, sizeof(line), "VOL %d%% %s",
             volumeB, playingB ? "PLAY" : "PAUSE");
    oled_print_at(6, line);

    // Show crossfader percentages
    snprintf(line, sizeof(line), "MIX: A %d%% B %d%%",
             mixA, mixB);
    oled_print_at(7, line);
}

// Close the DJ mixer display
void display_close(void)
{
    oled_close();
}