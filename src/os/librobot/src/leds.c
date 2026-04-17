#include "librobot.h"
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>

#define GPIO_DEVICE "/dev/gpiochip0" // Ruta completa requerida en v2

// Pines (offsets)
#define LED_PIN_POWER    27
#define LED_PIN_AUTO     21
#define LED_PIN_MANUAL   20
#define LED_PIN_OBSTACLE 16

static struct gpiod_chip *chip = NULL;
static struct gpiod_line_request *led_request = NULL;
static unsigned int led_offsets[] = {LED_PIN_POWER, LED_PIN_AUTO, LED_PIN_MANUAL, LED_PIN_OBSTACLE};
static robot_led_state_t led_states[4] = {LED_OFF, LED_OFF, LED_OFF, LED_OFF};

static int leds_init() {
    if (led_request) return 0;

    chip = gpiod_chip_open(GPIO_DEVICE);
    if (!chip) return -1;

    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    // Agregamos los 4 offsets con la misma configuración de salida
    gpiod_line_config_add_line_settings(line_cfg, led_offsets, 4, settings);

    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "robot_leds");

    led_request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    // Limpieza de objetos de configuración 
    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);

    if (!led_request) return -1;
    return 0;
}

robot_status_t robot_led_set(robot_led_t led, robot_led_state_t state) {
    if (leds_init() != 0) return ROBOT_ERR_HW;
    if (led < 0 || led > 3) return ROBOT_ERR_ARG;

    // En v2 usamos la petición y el offset específico
    gpiod_line_request_set_value(led_request, led_offsets[led], (enum gpiod_line_value)state);
    led_states[led] = state;
    return ROBOT_OK;
}

robot_led_state_t robot_led_get(robot_led_t led) {
    if (led < 0 || led > 3) return LED_OFF;
    return led_states[led];
}