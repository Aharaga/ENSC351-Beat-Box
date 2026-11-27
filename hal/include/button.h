
#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <stdbool.h>

// The four buttons on the DJ mixer
typedef enum {
    BUTTON_A_PLAY,
    BUTTON_A_NEXT,
    BUTTON_B_PLAY,
    BUTTON_B_NEXT,
    BUTTON_COUNT
} Button;

bool button_init(void);
bool button_is_button_pressed(Button button);
bool button_was_pressed(Button button);
void button_cleanup(void);

#endif
