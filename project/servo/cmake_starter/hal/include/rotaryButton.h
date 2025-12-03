#ifndef ROTARY_BUTTON_H
#define ROTARY_BUTTON_H

#include <stdbool.h>

//initialize rotary encoder pushbutton 
void RotaryButton_init(void);

//returns true if button is pressed
bool RotaryButton_wasPressed(void);

//rotary button cleanup
void RotaryButton_cleanup(void);

#endif

