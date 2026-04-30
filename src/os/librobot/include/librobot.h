#ifndef LIBROBOT_H
#define LIBROBOT_H
#define MS_PER_90_DEG 850 // Calibración base para rotación de 90 grados TODO: (ajustar según pruebas reales)

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdint.h>
#include <gpiod.h>


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

/* --- Estructuras de Datos --- */

// Unificamos Pose y Odometry para que tu compañero sea feliz
typedef struct {
    float x;          /* Posición estimada en X (cm) */
    float y;          /* Posición estimada en Y (cm) */
    float angle;      /* Orientación en grados */
} Pose;

typedef Pose robot_odometry_t;

typedef struct {
    float front_cm;   /* Distancia frontal en cm */
    float left_cm;    /* Distancia lateral izquierda en cm */
} robot_sensor_data_t;

/* --- Prototipos de Funciones --- */

// Sistema
robot_status_t robot_init(void);
robot_status_t robot_shutdown(void);
robot_status_t robot_set_mode(robot_mode_t mode);
robot_mode_t   robot_get_mode(void);

// Control de movimiento
// speed: 0 (parado) a 100 (velocidad máxima)
robot_status_t motors_init(void);
robot_status_t robot_move(robot_dir_t dir, uint8_t speed);
robot_status_t robot_stop(void);
robot_status_t robot_rotate(robot_dir_t dir, float degrees); // Requisito: Rotación
robot_status_t robot_get_odometry(robot_odometry_t *odom);

// Sensores
robot_status_t sensors_init(void);
robot_status_t robot_sensor_read(robot_sensor_data_t *data);

// LEDs
robot_status_t leds_init(void);
robot_status_t robot_led_set(robot_led_t led, robot_led_state_t state);
robot_led_state_t robot_led_get(robot_led_t led); // Requisito: Get LED

// Audio
robot_status_t robot_audio_play(const char *filepath);
robot_status_t robot_audio_pause(void);
robot_status_t robot_audio_stop(void);
robot_status_t robot_audio_set_volume(uint8_t volume);
robot_status_t robot_audio_get_list(char ***files, int *count);

// Callbacks (Obstáculos)
typedef void (*robot_obstacle_callback)(float distance);
void robot_set_obstacle_handler(robot_obstacle_callback cb);

typedef enum {
    SENSOR_FRONT = 0,
    SENSOR_LEFT = 1
} robot_sensor_t;

// Devuelve la distancia en centímetros, o -1.0f si falla o está fuera de rango.
float robot_get_distance(int sensor_id);



#ifdef __cplusplus
}
#endif

#endif