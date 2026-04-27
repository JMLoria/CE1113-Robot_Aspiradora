#ifndef BIBLIOTECA_ROBOT_H
#define BIBLIOTECA_ROBOT_H

extern "C"
{
    // Inicialización del hardware base antes de usar cualquier periférico del robot.
    void robot_init(); // Inicializa GPIOs y PWM

    // Control de tracción independiente por lado para maniobras y corrección de rumbo.
    void set_motors(int speed_left, int speed_right);

    // Lectura unificada de los sensores de proximidad expuestos por la capa OS.
    float read_sensor(int sensor_id);

    // Control de indicadores visuales para estados del sistema y alertas.
    void set_led(int led_id, bool state);
}

#endif // BIBLIOTECA_ROBOT_H