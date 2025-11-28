#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "hal/servo.h"

#define SERVO_MIN_US 1000     // 1.0 ms = 0 degrees
#define SERVO_MAX_US 2000     // 2.0 ms = 180 degrees
#define SERVO_PERIOD_US 20000 // 20 ms

static const char* pwm_path = "/dev/hat/pwm/GPIO12";

// Write to a PWM file
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
    // Set period
    pwm_write("period", SERVO_PERIOD_US * 1000);

    // Start disabled
    pwm_write("duty_cycle", 0);

    // Enable
    pwm_write("enable", 1);

    return 0;
}

void servo_set_angle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    int pulse_us = SERVO_MIN_US +
        (angle * (SERVO_MAX_US - SERVO_MIN_US)) / 180;

    pwm_write("duty_cycle", pulse_us * 1000);
}

void servo_disable() {
    pwm_write("enable", 0);
}
