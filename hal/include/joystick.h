#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdbool.h>

// Enum representing the joystick directions plus no direction
enum JoyDirection {
    JOY_UP,
    JOY_DOWN,
    JOY_LEFT,
    JOY_RIGHT,
    JOY_CENTER,
    JOY_NONE
};

void joystick_init(void);
void joystick_cleanup(void);

// Returns the current direction the joystick is pressed
enum JoyDirection joystick_get_direction(void);

#endif