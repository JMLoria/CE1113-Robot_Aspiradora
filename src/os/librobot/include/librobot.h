#ifndef LIBROBOT_H
#define LIBROBOT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Definiciones de tipos y constantes

typedef enum {
    ROBOT_OK       =  0,
    ROBOT_ERR_HW   = -1,  /* Error de hardware */
    ROBOT_ERR_ARG  = -2,  /* Argumento inválido */
    ROBOT_ERR_BUSY = -3   /* Recurso ocupado */
} robot_status_t;

typedef enum {
    DIR_FORWARD  = 0,
    DIR_BACKWARD = 1,
    DIR_LEFT     = 2,
    DIR_RIGHT    = 3,
    DIR_STOP     = 4
} robot_dir_t;

typedef enum {
    MODE_MANUAL    = 0,
    MODE_AUTONOMOUS = 1
} robot_mode_t;

typedef enum {
    LED_POWER     = 0,  /* Sistema encendido */
    LED_AUTO      = 1,  /* Modo autónomo activo */
    LED_MANUAL    = 2,  /* Modo manual activo */
    LED_OBSTACLE  = 3   /* Obstáculo detectado */
} robot_led_t;

typedef enum {
    LED_OFF = 0,
    LED_ON  = 1
} robot_led_state_t;

typedef struct {
    float front_cm;   /* Distancia frontal en cm */
    float left_cm;    /* Distancia lateral izquierda en cm */
} robot_sensor_data_t;

typedef struct {
    float x;          /* Posición estimada en X (cm) */
    float y;          /* Posición estimada en Y (cm) */
    float angle_deg;  /* Orientación en grados */
} robot_odometry_t;

// Prototipos de funciones
robot_status_t robot_init(void);
robot_status_t robot_shutdown(void);

// Control de movimiento
// speed: 0 (parado) a 100 (velocidad máxima)
robot_status_t robot_move(robot_dir_t dir, uint8_t speed);
robot_status_t robot_stop(void);
robot_status_t robot_get_odometry(robot_odometry_t *odom);

// Sensores
robot_status_t robot_sensor_read(robot_sensor_data_t *data);

// LEDs
robot_status_t robot_led_set(robot_led_t led, robot_led_state_t state);

//  Modos de operación
robot_status_t robot_set_mode(robot_mode_t mode);
robot_mode_t   robot_get_mode(void);

// Audio
// filepath: ruta absoluta al archivo MP3 */
robot_status_t robot_audio_play(const char *filepath);
robot_status_t robot_audio_pause(void);
robot_status_t robot_audio_stop(void);
robot_status_t robot_audio_set_volume(uint8_t volume); /* 0-100 */
robot_status_t robot_audio_get_list(char ***files, int *count);

#ifdef __cplusplus
}
#endif

#endif