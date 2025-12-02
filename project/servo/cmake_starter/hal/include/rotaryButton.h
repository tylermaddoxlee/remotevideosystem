#ifndef ROTARY_BUTTON_H
#define ROTARY_BUTTON_H

#include <stdbool.h>

// Initialize the rotary-encoder pushbutton (SW pin only)
void RotaryButton_init(void);

// Returns true ONLY on a "press" event (active-low falling edge)
bool RotaryButton_wasPressed(void);

// Release GPIO resources
void RotaryButton_cleanup(void);

#endif
