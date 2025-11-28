#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>

typedef enum {
    JS_CENTER = 0,
    JS_UP,
    JS_DOWN,
    JS_LEFT,
    JS_RIGHT
} js_dir_t;

int js_open(const char* dev);
int js_configure(int fd, unsigned mode, unsigned bits, unsigned speed);
int js_read_channel(int fd, int ch, unsigned speed_hz);
js_dir_t js_direction(int x, int y);
js_dir_t js_read_dir(int fd, unsigned speed_hz, int chX, int chY);
void js_wait_center(int fd, unsigned speed_hz, int chX, int chY);

#endif
