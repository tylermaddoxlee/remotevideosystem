// main.c - Joystick + UDP-controlled MG996 servo on BeagleY-AI

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "hal/joystick.h"
#include "hal/servo.h"

#define SPI_DEV "/dev/spidev0.0"  // MCP3208 SPI device
#define SPI_MODE 0
#define SPI_BITS 8
#define SPI_SPEED 1000000         // 1 MHz

#define JOY_CH_X 0     // MCP3208 channel for X-axis
#define JOY_CH_Y 1     // MCP3208 channel for Y-axis

#define UDP_PORT 12345

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

// Non-blocking check for UDP messages
// Non-blocking check for UDP messages
void udp_check() {
    char buf[64];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int n = recvfrom(udp_fd, buf, sizeof(buf)-1, MSG_DONTWAIT,
                     (struct sockaddr*)&client_addr, &addr_len);
    if (n > 0) {
        buf[n] = '\0';
        if (strcmp(buf, "LEFT") == 0) {
            current_angle -= 5;            // decrease angle
            if (current_angle < 0) current_angle = 0;  // limit to 0
            servo_set_angle(current_angle);
            printf("UDP LEFT  → angle = %d\n", current_angle);
        } else if (strcmp(buf, "RIGHT") == 0) {
            current_angle += 5;            // increase angle
            if (current_angle > 180) current_angle = 180; // limit to 180
            servo_set_angle(current_angle);
            printf("UDP RIGHT → angle = %d\n", current_angle);
        } else if (strcmp(buf, "STOP") == 0) {
            // Optional: do nothing, or hold current angle
        }
    }
}


int main(void) {
    signal(SIGINT, handle_sigint);

    printf("Initializing Joystick + Servo + UDP Control...\n");

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
    // 2. Initialize Servo
    //------------------------------------------------------------------
    if (servo_init() < 0) {
        printf("ERROR: servo_init() failed.\n");
        close(js_fd);
        return 1;
    }

    // Start centered
    current_angle = 90; // middle for 180 servo
    servo_set_angle(current_angle);

    //------------------------------------------------------------------
    // 3. Initialize UDP
    //------------------------------------------------------------------
    if (udp_init() < 0) {
        printf("ERROR: UDP init failed\n");
        servo_disable();
        close(js_fd);
        return 1;
    }

    printf("System ready. Use joystick or send UDP commands LEFT/RIGHT.\n");

    //------------------------------------------------------------------
    // 4. Main Loop
    //------------------------------------------------------------------
    while (running) {
        // Joystick control
        js_dir_t dir = js_read_dir(js_fd, SPI_SPEED, JOY_CH_X, JOY_CH_Y);
        if (dir == JS_LEFT) {
            if (current_angle > 0) current_angle -= 3;
            if (current_angle < 0) current_angle = 0;
            servo_set_angle(current_angle);
            printf("Joystick LEFT  → angle = %d\n", current_angle);
        } else if (dir == JS_RIGHT) {
            if (current_angle < 180) current_angle += 3;
            if (current_angle > 180) current_angle = 180;
            servo_set_angle(current_angle);
            printf("Joystick RIGHT → angle = %d\n", current_angle);
        }

        // Check UDP input
        udp_check();

        usleep(20000); // 50Hz update
    }

    //------------------------------------------------------------------
    // Cleanup
    //------------------------------------------------------------------
    servo_disable();
    close(js_fd);
    close(udp_fd);
    printf("\nExiting cleanly.\n");

    return 0;
}