// main.c - Joystick-controlled MG996 servo on BeagleY-AI

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "hal/joystick.h"
#include "hal/servo.h"

#define SPI_DEV "/dev/spidev0.0"     // adjust if your MCP3208 SPI device is different
#define SPI_MODE 0
#define SPI_BITS 8
#define SPI_SPEED 1000000            // 1 MHz is typical for MCP3208

#define JOY_CH_X 0     // MCP3208 channel for X-axis
#define JOY_CH_Y 1     // MCP3208 channel for Y-axis (unused here, but available)

static int running = 1;

// Handle Ctrl+C gracefully
void handle_sigint(int sig) {
    running = 0;
}

int main(void) {
    signal(SIGINT, handle_sigint);

    printf("Initializing Joystick + Servo Control...\n");

    //------------------------------------------------------------------
    // 1. Initialize Joystick SPI
    //------------------------------------------------------------------
    int js_fd = js_open(SPI_DEV);
    if (js_fd < 0) {
        printf("ERROR: Cannot open SPI device %s\n", SPI_DEV);
        return 1;
    }

    if (js_configure(js_fd, SPI_MODE, SPI_BITS, SPI_SPEED) < 0) {
        printf("ERROR: SPI configure failed.\n");
        close(js_fd);
        return 1;
    }

    //------------------------------------------------------------------
    // 2. Initialize Servo (GPIO12 → HAT pin 32 PWM)
    //------------------------------------------------------------------
    if (servo_init() < 0) {
        printf("ERROR: servo_init() failed.\n");
        close(js_fd);
        return 1;
    }

    printf("Servo + Joystick ready.\n");
    printf("Push joystick LEFT/RIGHT to move the servo.\n");
    printf("Press Ctrl+C to exit.\n");

    //------------------------------------------------------------------
    // 3. Main Loop
    //------------------------------------------------------------------
    int current_angle = 90;    // start centered
    servo_set_angle(current_angle);

    while (running) {
        js_dir_t dir = js_read_dir(js_fd, SPI_SPEED, JOY_CH_X, JOY_CH_Y);

        switch (dir) {
            case JS_LEFT:
                if (current_angle > 0) {
                    current_angle -= 3;      // smooth movement
                    if (current_angle < 0) current_angle = 0;
                    servo_set_angle(current_angle);
                    printf("LEFT  → angle = %d\n", current_angle);
                }
                break;

            case JS_RIGHT:
                if (current_angle < 180) {
                    current_angle += 3;
                    if (current_angle > 180) current_angle = 180;
                    servo_set_angle(current_angle);
                    printf("RIGHT → angle = %d\n", current_angle);
                }
                break;

            case JS_CENTER:
            default:
                // You can choose to auto-center, or do nothing:
                // servo_set_angle(90);
                break;
        }

        usleep(20000); // 50ms loop
    }

    //------------------------------------------------------------------
    // Cleanup
    //------------------------------------------------------------------
    servo_disable();
    close(js_fd);
    printf("\nExiting cleanly.\n");

    return 0;
}
