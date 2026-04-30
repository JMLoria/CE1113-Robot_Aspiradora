#include "librobot.h"
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#ifndef GPIO_CHIP_NAME
#define GPIO_CHIP_NAME "gpiochip0" // Valor por defecto para RPi4
#endif

#define GPIO_DEVICE "/dev/" GPIO_CHIP_NAME

// Offsets de la RPi5
#define MOTOR_L_IN1  17
#define MOTOR_L_IN2  27
#define MOTOR_R_IN1  22
#define MOTOR_R_IN2  23
#define MOTOR_L_PWM  12
#define MOTOR_R_PWM  13

#define PWM_PERIOD_US 20000


typedef struct {
    unsigned int       offset;
    uint8_t            duty_cycle;
    int                running;
    pthread_t          thread;
} motor_pwm_t;

static struct gpiod_chip *chip = NULL;
static struct gpiod_line_request *motor_request = NULL;
static motor_pwm_t pwm_left, pwm_right;

static void *pwm_thread(void *arg) {
    motor_pwm_t *m = (motor_pwm_t *)arg;
    while (m->running) {
        uint32_t on_time = (PWM_PERIOD_US * m->duty_cycle) / 100;
        uint32_t off_time = PWM_PERIOD_US - on_time;

        if (on_time > 0) {
            gpiod_line_request_set_value(motor_request, m->offset, GPIOD_LINE_VALUE_ACTIVE);
            usleep(on_time);
        }
        if (off_time > 0) {
            gpiod_line_request_set_value(motor_request, m->offset, GPIOD_LINE_VALUE_INACTIVE);
            usleep(off_time);
        }
    }
    gpiod_line_request_set_value(motor_request, m->offset, GPIOD_LINE_VALUE_INACTIVE);
    return NULL;
}

static bool initialized = false;

robot_status_t motors_init(void) {
    if (motor_request) return ROBOT_OK;

    chip = gpiod_chip_open(GPIO_DEVICE);
    if (!chip) return ROBOT_ERR_HW;

    unsigned int all_offsets[] = {MOTOR_L_IN1, MOTOR_L_IN2, MOTOR_R_IN1, MOTOR_R_IN2, MOTOR_L_PWM, MOTOR_R_PWM};
    
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, all_offsets, 6, settings);

    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "robot_motors");

    motor_request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);

    if (!motor_request) return ROBOT_ERR_HW;

    pwm_left.offset = MOTOR_L_PWM;
    pwm_left.running = 1;
    pwm_right.offset = MOTOR_R_PWM;
    pwm_right.running = 1;

    pthread_create(&pwm_left.thread, NULL, pwm_thread, &pwm_left);
    pthread_create(&pwm_right.thread, NULL, pwm_thread, &pwm_right);

    initialized = true;
    return ROBOT_OK;
}

robot_status_t robot_move(robot_dir_t dir, uint8_t speed) {
    if (motors_init() != ROBOT_OK) return ROBOT_ERR_HW;

    pwm_left.duty_cycle = (speed > 100) ? 100 : speed;
    pwm_right.duty_cycle = pwm_left.duty_cycle;

    // Ahora el tipo ya es conocido por el compilador gracias al cambio en librobot.h
    enum gpiod_line_value vals[4]; 
    
    switch (dir) {
        case DIR_FORWARD:  
            vals[0] = GPIOD_LINE_VALUE_ACTIVE;   vals[1] = GPIOD_LINE_VALUE_INACTIVE; 
            vals[2] = GPIOD_LINE_VALUE_ACTIVE;   vals[3] = GPIOD_LINE_VALUE_INACTIVE; 
            break;
        case DIR_BACKWARD: 
            vals[0] = GPIOD_LINE_VALUE_INACTIVE; vals[1] = GPIOD_LINE_VALUE_ACTIVE; 
            vals[2] = GPIOD_LINE_VALUE_INACTIVE; vals[3] = GPIOD_LINE_VALUE_ACTIVE; 
            break;
        case DIR_LEFT:     
            vals[0] = GPIOD_LINE_VALUE_INACTIVE; vals[1] = GPIOD_LINE_VALUE_ACTIVE; 
            vals[2] = GPIOD_LINE_VALUE_ACTIVE;   vals[3] = GPIOD_LINE_VALUE_INACTIVE; 
            break;
        case DIR_RIGHT:    
            vals[0] = GPIOD_LINE_VALUE_ACTIVE;   vals[1] = GPIOD_LINE_VALUE_INACTIVE; 
            vals[2] = GPIOD_LINE_VALUE_INACTIVE; vals[3] = GPIOD_LINE_VALUE_ACTIVE; 
            break;
        case DIR_STOP:     
            vals[0] = GPIOD_LINE_VALUE_INACTIVE; vals[1] = GPIOD_LINE_VALUE_INACTIVE; 
            vals[2] = GPIOD_LINE_VALUE_INACTIVE; vals[3] = GPIOD_LINE_VALUE_INACTIVE; 
            pwm_left.duty_cycle = 0; pwm_right.duty_cycle = 0; 
            break;
        default: return ROBOT_ERR_ARG;
    }

    unsigned int drv_offsets[] = {MOTOR_L_IN1, MOTOR_L_IN2, MOTOR_R_IN1, MOTOR_R_IN2};
    
    gpiod_line_request_set_values_subset(motor_request, 4, drv_offsets, vals);
    
    return ROBOT_OK;
}

robot_status_t robot_stop(void) {
    return robot_move(DIR_STOP, 0);
}

robot_status_t robot_rotate(robot_dir_t dir, float degrees) {
    // Verificación de seguridad 
    if (!initialized) return ROBOT_ERR_HW;
    if (dir != DIR_LEFT && dir != DIR_RIGHT) return ROBOT_ERR_ARG;
    
    // Cálculo proporcional
    uint32_t duration_ms = (uint32_t)((degrees / 90.0f) * MS_PER_90_DEG);
    
    // Ejecución física
    robot_move(dir, 60); 
    usleep(duration_ms * 1000);
    robot_stop(); 
    
    return ROBOT_OK;
}
