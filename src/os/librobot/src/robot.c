#include "librobot.h"
#include <stdio.h>
#include <stdlib.h>

/* Estado interno del robot */
static robot_mode_t current_mode = MODE_MANUAL;
static int initialized = 0;

static Pose current_pose = {0.0f, 0.0f, 0.0f};      // Memoria de posición
static robot_obstacle_callback obstacle_cb = NULL; // Memoria del callback

static pthread_t auto_thread;
static int stop_auto = 0;

// Funciones de inicialización y cierre
robot_status_t robot_init(void) {
    if (initialized) return ROBOT_ERR_BUSY;

    // Inicializar todos los subsistemas de hardware
    if (leds_init() != 0) return ROBOT_ERR_HW;
    if (sensors_init() != ROBOT_OK) return ROBOT_ERR_HW;
    if (motors_init() != ROBOT_OK) return ROBOT_ERR_HW;

    robot_led_set(LED_POWER, LED_ON);
    current_mode = MODE_MANUAL;
    
    // Lanzar el hilo de monitoreo autónomo
    stop_auto = 0;
    pthread_create(&auto_thread, NULL, autonomous_worker, NULL);

    initialized = 1;
    return ROBOT_OK;
}

// Función de cierre
robot_status_t robot_shutdown(void)
{
    if (!initialized) {
        return ROBOT_ERR_HW;
    }

    robot_stop();
    robot_audio_stop();

    robot_led_set(LED_POWER,    LED_OFF);
    robot_led_set(LED_AUTO,     LED_OFF);
    robot_led_set(LED_MANUAL,   LED_OFF);
    robot_led_set(LED_OBSTACLE, LED_OFF);

    initialized = 0;

    printf("robot_shutdown: sistema apagado correctamente\n");
    return ROBOT_OK;
}

robot_status_t robot_set_obstacle_callback(robot_obstacle_callback cb) {
    obstacle_cb = cb;
    return ROBOT_OK;
}

// Funciones de control de movimiento
robot_status_t robot_set_mode(robot_mode_t mode)
{
    if (mode != MODE_MANUAL && mode != MODE_AUTONOMOUS) {
        return ROBOT_ERR_ARG;
    }

    current_mode = mode;

    if (mode == MODE_AUTONOMOUS) {
        robot_led_set(LED_AUTO,   LED_ON);
        robot_led_set(LED_MANUAL, LED_OFF);
        robot_audio_play("/usr/share/robot/sounds/autonomous.mp3");
    } else {
        robot_led_set(LED_AUTO,   LED_OFF);
        robot_led_set(LED_MANUAL, LED_ON);
        robot_audio_play("/usr/share/robot/sounds/manual.mp3");
    }

    return ROBOT_OK;
}

// Función para obtener el modo actual
robot_mode_t robot_get_mode(void)
{
    return current_mode;
}


robot_status_t robot_get_odometry(robot_odometry_t *odom) {
    if (!odom) return ROBOT_ERR_ARG;
    *odom = current_pose;
    return ROBOT_OK;
}

// Callbacks de Obstáculos 
void robot_set_obstacle_handler(robot_obstacle_callback cb) {
    obstacle_cb = cb;
}

// Función interna para disparar el evento
void robot_internal_notify_obstacle(float distance) {
    if (obstacle_cb) {
        obstacle_cb(distance);
    }
}
// Hilo de control autónomo
void* autonomous_worker(void* arg) {
    while (!stop_auto) {
        if (current_mode == MODE_AUTONOMOUS) {
            float dist = robot_get_distance(SENSOR_FRONT); /
            
            if (dist < 15.0f && dist > 0.0f) { // Obstáculo a menos de 15cm
                robot_stop();
                robot_audio_play("obstacle_detected.mp3"); // Notificación inmediata
                
                // Disparar callback de obstáculo para que la aplicación pueda reaccionar (ej: actualizar mapa)
                if (obstacle_cb) {
                    obstacle_cb(dist);
                }
                
                // Lógica reactiva simple: retroceder y girar
                robot_move(DIR_BACKWARD, 60);
                usleep(500000);
                robot_move(DIR_LEFT, 80);
                usleep(MS_PER_90_DEG * 1000); 
            } else {
                robot_move(DIR_FORWARD, 70);
            }
        }
        usleep(100000); // Muestreo cada 100ms para no saturar la CPU
    }
    return NULL;
}