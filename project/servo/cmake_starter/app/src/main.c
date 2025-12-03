// main.c - Joystick + UDP-controlled MG996 servo on BeagleY-AI + Doorbell Sound

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "hal/joystick.h"
#include "hal/servo.h"
#include "hal/rotaryButton.h"

#define SPI_DEV "/dev/spidev0.0"  // MCP3208 SPI device
#define SPI_MODE 0
#define SPI_BITS 8
#define SPI_SPEED 1000000 // 1 MHz

#define JOY_CH_X 1
#define JOY_CH_Y 0

#define UDP_PORT 12345

// Doorbell sound file
#define DOORBELL_SOUND_WAV "/mnt/remote/doorbell.wav"

static int running = 1;

// UDP variables
static int udp_fd;
static struct sockaddr_in udp_addr;

// Servo state
static int current_angle = 90;

// Handle Ctrl+C gracefully
void handle_sigint(int sig) {
    running = 0;
}

// Initialize UDP socket
int udp_init() {
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) {
        perror("socket");
        return -1;
    }

    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(UDP_PORT);

    if (bind(udp_fd, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("bind");
        close(udp_fd);
        return -1;
    }

    return 0;
}

// UDP check
void udp_check() {
    char buf[64];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int n = recvfrom(udp_fd, buf, sizeof(buf)-1, MSG_DONTWAIT,
                     (struct sockaddr*)&client_addr, &addr_len);
    if (n > 0) {
        buf[n] = '\0';

        if (strcmp(buf, "LEFT") == 0) {
            current_angle -= 5;
            if (current_angle < 0) current_angle = 0;
            servo_set_angle(current_angle);
            printf("UDP LEFT → angle = %d\n", current_angle);

        } else if (strcmp(buf, "RIGHT") == 0) {
            current_angle += 5;
            if (current_angle > 180) current_angle = 180;
            servo_set_angle(current_angle);
            printf("UDP RIGHT → angle = %d\n", current_angle);

        } else if (strcmp(buf, "STOP") == 0) {
        }
    }
}

int main(void) {
    signal(SIGINT, handle_sigint);

    printf("Initializing Joystick + Servo + UDP Control...\n");

    // Joystick SPI
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

    // servo
    if (servo_init() < 0) {
        printf("ERROR: servo_init() failed.\n");
        close(js_fd);
        return 1;
    }

    current_angle = 90;
    servo_set_angle(current_angle);

    // ROTARY BUTTON
    RotaryButton_init();

    // UDP
    if (udp_init() < 0) {
        printf("ERROR: UDP init failed\n");
        servo_disable();
        close(js_fd);
        return 1;
    }

    printf("System ready. Joystick, UDP, and Doorbell active.\n");

    // Main loop
    while (running) {

        // PLAY DOORBELL SOUND
        if (RotaryButton_wasPressed()) {
            printf("Doorbell pressed!\n");

            char cmd[256];
            snprintf(cmd, sizeof(cmd), "aplay %s &", DOORBELL_SOUND_WAV);
            system(cmd);
        }

        // Joystick angle control
        js_dir_t dir = js_read_dir(js_fd, SPI_SPEED, JOY_CH_X, JOY_CH_Y);
        if (dir == JS_LEFT) {
            if (current_angle > 0) current_angle -= 3;
            servo_set_angle(current_angle);
            printf("Joystick LEFT → angle = %d\n", current_angle);

        } else if (dir == JS_RIGHT) {
            if (current_angle < 180) current_angle += 3;
            servo_set_angle(current_angle);
            printf("Joystick RIGHT → angle = %d\n", current_angle);
        }

        // UDP messages
        udp_check();

        usleep(20000);
    }

    // Cleanup
    servo_disable();
    RotaryButton_cleanup();
    close(js_fd);
    close(udp_fd);

    printf("\nExiting cleanly.\n");
    return 0;
}
