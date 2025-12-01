#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "hal/servo.h"

#define SERVO_MIN_US 1000
#define SERVO_MAX_US 2000
#define SERVO_PERIOD_US 20000

static const char* pwm_path = "/dev/hat/pwm/GPIO12";

static void pwm_write(const char* name, int value) {
    char path[128];
    snprintf(path, sizeof(path), "%s/%s", pwm_path, name);

    int fd = open(path, O_WRONLY);
    if (fd >= 0) {
        char buf[32];
        int n = snprintf(buf, sizeof(buf), "%d", value);
        write(fd, buf, n);
        close(fd);
    }
}

int servo_init() {
    pwm_write("period", SERVO_PERIOD_US * 1000);
    pwm_write("duty_cycle", 1500000); // start in middle
    pwm_write("enable", 1);
    return 0;
}

// For continuous rotation, map 0–360 to pulse width
void servo_set_angle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 360) angle = 360;

    int pulse_us = SERVO_MIN_US + ((angle * (SERVO_MAX_US - SERVO_MIN_US)) / 360);
    pwm_write("duty_cycle", pulse_us * 1000);
}

void servo_disable() {
    pwm_write("enable", 0);
}
