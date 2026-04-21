#ifndef BIBLIOTECA_ROBOT_H
#define BIBLIOTECA_ROBOT_H

extern "C" {
    // --- Control de Sistema ---
    void robot_init(); // Inicializa GPIOs y PWM

    // --- Navegacion y Traccion ---
    void set_motors(int speed_left, int speed_right);
 
    // --- Sensores de Proximidad ---
    float read_sensor(int sensor_id);

    // --- Indicadores Visuales
    void set_led(int led_id, bool state);
}

#endif // BIBLIOTECA_ROBOT_H