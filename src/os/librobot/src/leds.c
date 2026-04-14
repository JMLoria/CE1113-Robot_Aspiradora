#include "librobot.h"
#include <gpiod.h>
#include <stdio.h>

#define GPIO_CHIP "gpiochip0"
// Pines físicos para voltímetro:
#define LED_PIN_POWER    27
#define LED_PIN_AUTO     21
#define LED_PIN_MANUAL   20
#define LED_PIN_OBSTACLE 16

static struct gpiod_chip *chip = NULL;
static struct gpiod_line *led_lines[4];
static robot_led_state_t led_states[4] = {LED_OFF, LED_OFF, LED_OFF, LED_OFF};

static int leds_init() {
    if (chip) return 0;
    chip = gpiod_chip_open_by_name(GPIO_CHIP);
    if (!chip) return -1;

    int pins[] = {LED_PIN_POWER, LED_PIN_AUTO, LED_PIN_MANUAL, LED_PIN_OBSTACLE};
    for(int i = 0; i < 4; i++) {
        led_lines[i] = gpiod_chip_get_line(chip, pins[i]);
        gpiod_line_request_output(led_lines[i], "robot_leds", 0);
    }
    return 0;
}

robot_status_t robot_led_set(robot_led_t led, robot_led_state_t state) {
    if (leds_init() != 0) return ROBOT_ERR_HW;
    if (led < 0 || led > 3) return ROBOT_ERR_ARG;

    gpiod_line_set_value(led_lines[led], (int)state);
    led_states[led] = state;
    return ROBOT_OK;
}

robot_led_state_t robot_led_get(robot_led_t led) {
    if (led < 0 || led > 3) return LED_OFF;
    return led_states[led];
}