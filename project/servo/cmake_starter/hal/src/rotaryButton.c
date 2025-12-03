#include "hal/rotaryButton.h"
#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#define ROTARY_BUTTON_CHIP "/dev/gpiochip2"
#define ROTARY_BUTTON_LINE 9 // GPIO21

static struct gpiod_chip *chip = NULL;
static struct gpiod_line_request *req = NULL;

static int lastState = 1;
static bool initialized = false;
// sets gpio pin input as button input
void RotaryButton_init(void)
{
    chip = gpiod_chip_open(ROTARY_BUTTON_CHIP);
    if (!chip) { perror("gpiod_chip_open"); return; }
    //if cant open, stop function
    unsigned int offset = ROTARY_BUTTON_LINE;

    struct gpiod_line_settings *s = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(s, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_bias(s, GPIOD_LINE_BIAS_PULL_UP);

    struct gpiod_line_config *lc = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(lc, &offset, 1, s);

    struct gpiod_request_config *rc = gpiod_request_config_new();
    gpiod_request_config_set_consumer(rc, "doorbell_button");

    req = gpiod_chip_request_lines(chip, rc, lc);

    initialized = true;
    printf("Doorbell button ready.\n");
}
//conditions if button is pressed
bool RotaryButton_wasPressed(void)
{
    if (!initialized) return false;

    int current = gpiod_line_request_get_value(req, ROTARY_BUTTON_LINE);
    if (current < 0) return false;

    bool pressed = (lastState == 1 && current == 0);
    lastState = current;

    return pressed;
}
//rotary button cleanup
void RotaryButton_cleanup(void)
{
    if (req) gpiod_line_request_release(req);
    if (chip) gpiod_chip_close(chip);
    initialized = false;

}