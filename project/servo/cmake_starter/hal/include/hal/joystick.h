#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>

// Joystick direction enum
typedef enum {
    JS_CENTER = 0,
    JS_UP,
    JS_DOWN,
    JS_LEFT,
    JS_RIGHT
} js_dir_t;

// open joystick spi 
int js_open(const char* dev);

// configure joystick settings
int js_configure(int fd, unsigned mode, unsigned bits, unsigned speed);

// reads joystick analog channel
int js_read_channel(int fd, int ch, unsigned speed_hz);

// converts adc value to direction enum
js_dir_t js_direction(int x, int y);

// returns joystick channels and direction
js_dir_t js_read_dir(int fd, unsigned speed_hz, int chX, int chY);

// block input until joystick centered
void js_wait_center(int fd, unsigned speed_hz, int chX, int chY);

#endif
