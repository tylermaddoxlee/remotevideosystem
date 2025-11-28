#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>

#include "hal/joystick.h"

#define CENTER   2048
#define DEADZONE 300

//--------------------------------------------------
// SPI OPEN
//--------------------------------------------------
int js_open(const char* dev) {
    return open(dev, O_RDWR);
}

//--------------------------------------------------
// SPI CONFIGURE
//--------------------------------------------------
int js_configure(int fd, unsigned mode, unsigned bits, unsigned speed) {
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) return -1;
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) return -1;
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) return -1;
    return 0;
}

//--------------------------------------------------
// READ MCP3208 CHANNEL
//--------------------------------------------------
int js_read_channel(int fd, int ch, unsigned speed_hz) {
    if (ch < 0 || ch > 7) return -1;

    uint8_t tx[3];
    uint8_t rx[3];

    tx[0] = 0x06 | ((ch & 0x04) >> 2);
    tx[1] = (ch & 0x03) << 6;
    tx[2] = 0x00;

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = speed_hz,
        .bits_per_word = 8,
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 0)
        return -1;

    int value = ((rx[1] & 0x0F) << 8) | rx[2];
    return value;
}

//--------------------------------------------------
// DIRECTION DETECTION
//--------------------------------------------------
js_dir_t js_direction(int x, int y) {
    if (x < (CENTER - DEADZONE)) return JS_LEFT;
    if (x > (CENTER + DEADZONE)) return JS_RIGHT;

    if (y < (CENTER - DEADZONE)) return JS_UP;
    if (y > (CENTER + DEADZONE)) return JS_DOWN;

    return JS_CENTER;
}

//--------------------------------------------------
// READ X/Y AND RETURN DIRECTION
//--------------------------------------------------
js_dir_t js_read_dir(int fd, unsigned speed_hz, int chX, int chY) {
    int x = js_read_channel(fd, chX, speed_hz);
    int y = js_read_channel(fd, chY, speed_hz);

    if (x < 0 || y < 0) return JS_CENTER;

    return js_direction(x, y);
}

//--------------------------------------------------
// WAIT FOR CENTER
//--------------------------------------------------
void js_wait_center(int fd, unsigned speed_hz, int chX, int chY) {
    while (1) {
        js_dir_t d = js_read_dir(fd, speed_hz, chX, chY);
        if (d == JS_CENTER) return;
        usleep(20000);
    }
}
