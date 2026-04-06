#include "librobot.h"
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>

// Chip GPIO de la RPi4
#define GPIO_CHIP    "gpiochip0"

// Pines de dirección (ajustar según circuito físico)
#define MOTOR_L_IN1  17
#define MOTOR_L_IN2  18
#define MOTOR_R_IN1  22
#define MOTOR_R_IN2  23

// Pines PWM por software
#define MOTOR_L_PWM  24
#define MOTOR_R_PWM  25

// Período PWM en microsegundos (50Hz)
#define PWM_PERIOD_US 20000

// Estado interno de un motor
typedef struct {
    struct gpiod_line *pwm_line;
    uint8_t            duty_cycle; // 0-100
    int                running;
    pthread_t          thread;
} motor_pwm_t;

static struct gpiod_chip *chip       = NULL;
static struct gpiod_line *line_l_in1 = NULL;
static struct gpiod_line *line_l_in2 = NULL;
static struct gpiod_line *line_r_in1 = NULL;
static struct gpiod_line *line_r_in2 = NULL;
static motor_pwm_t        pwm_left;
static motor_pwm_t        pwm_right;

// Hilo que genera la señal PWM por software
static void *pwm_thread(void *arg)
{
    motor_pwm_t *m = (motor_pwm_t *)arg;

    while (m->running) {
        uint32_t on_time  = (PWM_PERIOD_US * m->duty_cycle) / 100;
        uint32_t off_time = PWM_PERIOD_US - on_time;

        if (on_time > 0) {
            gpiod_line_set_value(m->pwm_line, 1);
            usleep(on_time);
        }
        if (off_time > 0) {
            gpiod_line_set_value(m->pwm_line, 0);
            usleep(off_time);
        }
    }

    gpiod_line_set_value(m->pwm_line, 0);
    return NULL;
}

static robot_status_t pwm_start(motor_pwm_t *m, struct gpiod_line *line, uint8_t duty)
{
    m->pwm_line   = line;
    m->duty_cycle = duty;
    m->running    = 1;

    if (pthread_create(&m->thread, NULL, pwm_thread, m) != 0) {
        fprintf(stderr, "motors: fallo al crear hilo PWM\n");
        return ROBOT_ERR_HW;
    }

    return ROBOT_OK;
}

static void pwm_stop(motor_pwm_t *m)
{
    m->running = 0;
    pthread_join(m->thread, NULL);
}

static robot_status_t motors_init(void)
{
    chip = gpiod_chip_open_by_name(GPIO_CHIP);
    if (!chip) {
        fprintf(stderr, "motors: no se pudo abrir %s\n", GPIO_CHIP);
        return ROBOT_ERR_HW;
    }

    line_l_in1 = gpiod_chip_get_line(chip, MOTOR_L_IN1);
    line_l_in2 = gpiod_chip_get_line(chip, MOTOR_L_IN2);
    line_r_in1 = gpiod_chip_get_line(chip, MOTOR_R_IN1);
    line_r_in2 = gpiod_chip_get_line(chip, MOTOR_R_IN2);

    struct gpiod_line *pwm_l = gpiod_chip_get_line(chip, MOTOR_L_PWM);
    struct gpiod_line *pwm_r = gpiod_chip_get_line(chip, MOTOR_R_PWM);

    if (!line_l_in1 || !line_l_in2 || !line_r_in1 || !line_r_in2 || !pwm_l || !pwm_r) {
        fprintf(stderr, "motors: fallo al obtener líneas GPIO\n");
        return ROBOT_ERR_HW;
    }

    gpiod_line_request_output(line_l_in1, "robot", 0);
    gpiod_line_request_output(line_l_in2, "robot", 0);
    gpiod_line_request_output(line_r_in1, "robot", 0);
    gpiod_line_request_output(line_r_in2, "robot", 0);
    gpiod_line_request_output(pwm_l,      "robot", 0);
    gpiod_line_request_output(pwm_r,      "robot", 0);

    pwm_start(&pwm_left,  pwm_l, 0);
    pwm_start(&pwm_right, pwm_r, 0);

    return ROBOT_OK;
}

static void motors_set_direction(int l_in1, int l_in2, int r_in1, int r_in2)
{
    gpiod_line_set_value(line_l_in1, l_in1);
    gpiod_line_set_value(line_l_in2, l_in2);
    gpiod_line_set_value(line_r_in1, r_in1);
    gpiod_line_set_value(line_r_in2, r_in2);
}

robot_status_t robot_move(robot_dir_t dir, uint8_t speed)
{
    if (!chip) {
        if (motors_init() != ROBOT_OK)
            return ROBOT_ERR_HW;
    }

    if (speed > 100) speed = 100;

    // Actualizar duty cycle de ambos motores
    pwm_left.duty_cycle  = speed;
    pwm_right.duty_cycle = speed;

    switch (dir) {
        case DIR_FORWARD:
            motors_set_direction(1, 0, 1, 0);
            break;
        case DIR_BACKWARD:
            motors_set_direction(0, 1, 0, 1);
            break;
        case DIR_LEFT:
            // Motor derecho adelante, izquierdo atrás
            motors_set_direction(0, 1, 1, 0);
            break;
        case DIR_RIGHT:
            // Motor izquierdo adelante, derecho atrás
            motors_set_direction(1, 0, 0, 1);
            break;
        case DIR_STOP:
            motors_set_direction(0, 0, 0, 0);
            pwm_left.duty_cycle  = 0;
            pwm_right.duty_cycle = 0;
            break;
        default:
            return ROBOT_ERR_ARG;
    }

    return ROBOT_OK;
}

robot_status_t robot_stop(void)
{
    return robot_move(DIR_STOP, 0);
}

robot_status_t robot_get_odometry(robot_odometry_t *odom)
{
    if (!odom) return ROBOT_ERR_ARG;

    // TODO: implementar con encoders o estimación por tiempo
    odom->x         = 0.0f;
    odom->y         = 0.0f;
    odom->angle_deg = 0.0f;

    return ROBOT_OK;
}