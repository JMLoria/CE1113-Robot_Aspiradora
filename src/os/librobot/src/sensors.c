#include "librobot.h"
#include <stdio.h>

// Forward declarations for gpiod types
struct gpiod_chip;
struct gpiod_line;
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

// Chip GPIO de la RPi4
#define GPIO_CHIP        "gpiochip0"

// Pines del sensor frontal HC-SR04
#define SENSOR_FRONT_TRIG  5
#define SENSOR_FRONT_ECHO  6

// Pines del sensor lateral HC-SR04
#define SENSOR_LEFT_TRIG   19
#define SENSOR_LEFT_ECHO   26

// Distancia máxima válida en cm
#define MAX_DISTANCE_CM    400.0f

// Timeout de espera del echo en microsegundos (30ms)
#define ECHO_TIMEOUT_US    30000

static struct gpiod_chip *chip            = NULL;
static struct gpiod_line *front_trig_line = NULL;
static struct gpiod_line *front_echo_line = NULL;
static struct gpiod_line *left_trig_line  = NULL;
static struct gpiod_line *left_echo_line  = NULL;

static robot_status_t sensors_init(void)
{
    chip = gpiod_chip_open_by_name(GPIO_CHIP);
    if (!chip) {
        fprintf(stderr, "sensors: no se pudo abrir %s\n", GPIO_CHIP);
        return ROBOT_ERR_HW;
    }

    front_trig_line = gpiod_chip_get_line(chip, SENSOR_FRONT_TRIG);
    front_echo_line = gpiod_chip_get_line(chip, SENSOR_FRONT_ECHO);
    left_trig_line  = gpiod_chip_get_line(chip, SENSOR_LEFT_TRIG);
    left_echo_line  = gpiod_chip_get_line(chip, SENSOR_LEFT_ECHO);

    if (!front_trig_line || !front_echo_line || !left_trig_line || !left_echo_line) {
        fprintf(stderr, "sensors: fallo al obtener líneas GPIO\n");
        return ROBOT_ERR_HW;
    }

    gpiod_line_request_output(front_trig_line, "robot", 0);
    gpiod_line_request_input(front_echo_line,  "robot");
    gpiod_line_request_output(left_trig_line,  "robot", 0);
    gpiod_line_request_input(left_echo_line,   "robot");

    return ROBOT_OK;
}

// Mide la distancia en cm de un sensor HC-SR04
static float measure_distance(struct gpiod_line *trig, struct gpiod_line *echo)
{
    struct timespec start, end;
    uint32_t elapsed_us = 0;

    // Enviar pulso de 10us en el trigger
    gpiod_line_set_value(trig, 1);
    struct timespec pulse = {0, 10000}; // 10 microsegundos
    nanosleep(&pulse, NULL);
    gpiod_line_set_value(trig, 0);

    // Esperar flanco de subida del echo
    uint32_t timeout = ECHO_TIMEOUT_US;
    while (gpiod_line_get_value(echo) == 0 && timeout--) {
        struct timespec wait = {0, 1000}; // 1 microsegundo
        nanosleep(&wait, NULL);
    }
    if (timeout == 0) {
        fprintf(stderr, "sensors: timeout esperando echo (subida)\n");
        return -1.0f;
    }
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Esperar flanco de bajada del echo
    timeout = ECHO_TIMEOUT_US;
    while (gpiod_line_get_value(echo) == 1 && timeout--) {
        struct timespec wait = {0, 1000};
        nanosleep(&wait, NULL);
    }
    if (timeout == 0) {
        fprintf(stderr, "sensors: timeout esperando echo (bajada)\n");
        return -1.0f;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calcular distancia
    // elapsed en microsegundos
    elapsed_us = (end.tv_sec  - start.tv_sec)  * 1000000 +
                 (end.tv_nsec - start.tv_nsec) / 1000;

    // Distancia = (tiempo * velocidad del sonido) / 2
    // Velocidad del sonido = 0.0343 cm/us
    float distance_cm = (elapsed_us * 0.0343f) / 2.0f;

    if (distance_cm > MAX_DISTANCE_CM) {
        return MAX_DISTANCE_CM;
    }

    return distance_cm;
}

robot_status_t robot_sensor_read(robot_sensor_data_t *data)
{
    if (!data) return ROBOT_ERR_ARG;

    if (!chip) {
        if (sensors_init() != ROBOT_OK)
            return ROBOT_ERR_HW;
    }

    data->front_cm = measure_distance(front_trig_line, front_echo_line);
    data->left_cm  = measure_distance(left_trig_line,  left_echo_line);

    if (data->front_cm < 0 || data->left_cm < 0) {
        return ROBOT_ERR_HW;
    }

    return ROBOT_OK;
}