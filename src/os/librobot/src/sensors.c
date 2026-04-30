#define _POSIX_C_SOURCE 199309L
#include <gpiod.h>
#include "librobot.h"
#include <stdio.h>
#include <time.h>

#ifndef GPIO_CHIP_NAME
#define GPIO_CHIP_NAME "gpiochip0"
#endif

#define GPIO_DEVICE "/dev/" GPIO_CHIP_NAME

#define SENSOR_FRONT_TRIG  5
#define SENSOR_FRONT_ECHO  6
#define SENSOR_LEFT_TRIG   20
#define SENSOR_LEFT_ECHO   21

static struct gpiod_chip *chip = NULL;
static struct gpiod_line_request *sensor_request = NULL;

// Función de inicialización de sensores
robot_status_t sensors_init(void) {
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

// Función auxiliar para calcular microsegundos transcurridos de forma exacta
static double get_elapsed_us(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * 1000000.0 + (end->tv_nsec - start->tv_nsec) / 1000.0;
}

// Nueva implementación de lectura con Timeouts estrictos
static float measure_distance(unsigned int trig, unsigned int echo) {
    struct timespec start, end, timeout_start, current;
    
    // 1. Enviar Pulso Trigger de 10us
    gpiod_line_request_set_value(sensor_request, trig, GPIOD_LINE_VALUE_ACTIVE);
    struct timespec pulse = {0, 10000};
    nanosleep(&pulse, NULL);
    gpiod_line_request_set_value(sensor_request, trig, GPIOD_LINE_VALUE_INACTIVE);

    // 2. Esperar flanco de subida (Timeout de 30ms)
    clock_gettime(CLOCK_MONOTONIC, &timeout_start);
    while (gpiod_line_request_get_value(sensor_request, echo) == GPIOD_LINE_VALUE_INACTIVE) {
        clock_gettime(CLOCK_MONOTONIC, &current);
        if (get_elapsed_us(&timeout_start, &current) > 30000.0) return -1.0f; // Fuera de rango / Error hardware
    }
    clock_gettime(CLOCK_MONOTONIC, &start); // Inicia cronómetro de viaje del sonido

    // 3. Esperar flanco de bajada (Timeout de 30ms)
    while (gpiod_line_request_get_value(sensor_request, echo) == GPIOD_LINE_VALUE_ACTIVE) {
        clock_gettime(CLOCK_MONOTONIC, &current);
        if (get_elapsed_us(&start, &current) > 30000.0) return -1.0f; // Objeto demasiado lejos
    }
    clock_gettime(CLOCK_MONOTONIC, &end); // Termina cronómetro

    // 4. Calcular distancia matemática
    double elapsed_us = get_elapsed_us(&start, &end);
    
    // Formula: Distancia = (Tiempo * Velocidad) / 2
    // Velocidad del sonido = 0.0343 cm/us
    float distance = (float)(elapsed_us * 0.0343 / 2.0);
    
    return distance;
}

float robot_get_distance(int sensor_id) {
    if (!sensor_request) return -1.0f;

    // Asumimos que 0 es el Frente y 1 es la Izquierda según la declaración en robot.c
    if (sensor_id == 0) { 
        return measure_distance(SENSOR_FRONT_TRIG, SENSOR_FRONT_ECHO);
    } else if (sensor_id == 1) { 
        return measure_distance(SENSOR_LEFT_TRIG, SENSOR_LEFT_ECHO);
    }
    
    return -1.0f;
}

robot_status_t robot_sensor_read(robot_sensor_data_t *data) {
    if (!data) return ROBOT_ERR_ARG;

    // Asignamos a los campos con sufijo _cm
    data->front_cm = robot_get_distance(SENSOR_FRONT);
    data->left_cm  = robot_get_distance(SENSOR_LEFT);

    return ROBOT_OK;
}