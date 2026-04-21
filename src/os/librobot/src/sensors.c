#define _POSIX_C_SOURCE 199309L
#include <gpiod.h>
#include "librobot.h"
#include <stdio.h>
#include <time.h>

#define GPIO_DEVICE "/dev/gpiochip0"
#define SENSOR_FRONT_TRIG  5
#define SENSOR_FRONT_ECHO  6
#define SENSOR_LEFT_TRIG   19
#define SENSOR_LEFT_ECHO   26

static struct gpiod_chip *chip = NULL;
static struct gpiod_line_request *sensor_request = NULL;

static robot_status_t sensors_init(void) {
    if (sensor_request) return ROBOT_OK;

    chip = gpiod_chip_open(GPIO_DEVICE);
    if (!chip) return ROBOT_ERR_HW;

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    
    // Configuración para Triggers (Salida)
    struct gpiod_line_settings *out_settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(out_settings, GPIOD_LINE_DIRECTION_OUTPUT);
    unsigned int trigs[] = {SENSOR_FRONT_TRIG, SENSOR_LEFT_TRIG};
    gpiod_line_config_add_line_settings(line_cfg, trigs, 2, out_settings);

    // Configuración para Echoes (Entrada)
    struct gpiod_line_settings *in_settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(in_settings, GPIOD_LINE_DIRECTION_INPUT);
    unsigned int echoes[] = {SENSOR_FRONT_ECHO, SENSOR_LEFT_ECHO};
    gpiod_line_config_add_line_settings(line_cfg, echoes, 2, in_settings);

    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "robot_sensors");

    sensor_request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    gpiod_line_settings_free(out_settings);
    gpiod_line_settings_free(in_settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);

    return sensor_request ? ROBOT_OK : ROBOT_ERR_HW;
}

static float measure_distance(unsigned int trig, unsigned int echo) {
    struct timespec start, end;
    // Pulso Trigger
    gpiod_line_request_set_value(sensor_request, trig, GPIOD_LINE_VALUE_ACTIVE);
    struct timespec pulse = {0, 10000};
    nanosleep(&pulse, NULL);
    gpiod_line_request_set_value(sensor_request, trig, GPIOD_LINE_VALUE_INACTIVE);

    // Esperar flanco subida
    int timeout = 30000;
    while (gpiod_line_request_get_value(sensor_request, echo) == GPIOD_LINE_VALUE_INACTIVE && timeout--) {
        struct timespec wait = {0, 1000};
        nanosleep(&wait, NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Esperar flanco bajada
    timeout = 30000;
    while (gpiod_line_request_get_value(sensor_request, echo) == GPIOD_LINE_VALUE_ACTIVE && timeout--) {
        struct timespec wait = {0, 1000};
        nanosleep(&wait, NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed_us = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3;
    return (float)(elapsed_us * 0.0343) / 2.0f;
}

robot_status_t robot_sensor_read(robot_sensor_data_t *data) {
    if (sensors_init() != ROBOT_OK) return ROBOT_ERR_HW;
    data->front_cm = measure_distance(SENSOR_FRONT_TRIG, SENSOR_FRONT_ECHO);
    data->left_cm  = measure_distance(SENSOR_LEFT_TRIG, SENSOR_LEFT_ECHO);
    return ROBOT_OK;
}