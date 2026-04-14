#include "librobot.h"
#include <stdio.h>
#include <unistd.h>

void wait_for_user(const char* msg) {
    printf("\n[PRUEBA] %s", msg);
    printf("\nPresiona ENTER cuando tengas el voltímetro listo...");
    getchar();
}

int main() {
    printf("=== DIAGNÓSTICO DE HARDWARE - ROBOT ASPIRADORA ===\n");

    if (robot_init() != ROBOT_OK) {
        printf("Error inicializando el sistema.\n");
        return -1;
    }

    // --- PRUEBA DE LEDS ---
    printf("\n--- Fase 1: LEDs (Esperado: 3.3V en el pin correspondiente) ---");
    
    robot_led_set(LED_POWER, LED_ON);
    wait_for_user("Verificando LED_POWER (Pin 27)");
    
    robot_led_set(LED_AUTO, LED_ON);
    wait_for_user("Verificando LED_AUTO (Pin 22* - ¡Atención con el conflicto!)");
    
    robot_led_set(LED_MANUAL, LED_ON);
    wait_for_user("Verificando LED_MANUAL (Pin 10)");
    
    robot_led_set(LED_OBSTACLE, LED_ON);
    wait_for_user("Verificando LED_OBSTACLE (Pin 9)");

    // --- PRUEBA DE MOTORES (DIRECCIÓN) ---
    printf("\n\n--- Fase 2: Motores Dirección (Esperado: 3.3V en un pin, 0V en el otro) ---");
    
    robot_move(DIR_FORWARD, 100);
    wait_for_user("Hacia ADELANTE: Revisa L_IN1(17), L_IN2(18), R_IN1(22), R_IN2(23)");
    
    robot_move(DIR_BACKWARD, 100);
    wait_for_user("Hacia ATRÁS: Los voltajes de los pines IN deben invertirse");
    
    robot_stop();

    // --- PRUEBA DE MOTORES (PWM/VELOCIDAD) ---
    printf("\n\n--- Fase 3: Motores PWM (Esperado: Variación de voltaje promedio) ---");
    
    robot_move(DIR_FORWARD, 100);
    wait_for_user("PWM al 100%: Revisa L_PWM(24) y R_PWM(25) -> Debe dar ~3.3V");
    
    robot_move(DIR_FORWARD, 50);
    wait_for_user("PWM al 50%: Revisa L_PWM(24) y R_PWM(25) -> Debe dar ~1.65V");

    robot_shutdown();
    printf("\n=== DIAGNÓSTICO FINALIZADO ===\n");
    return 0;
}