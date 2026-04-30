# Guía de Ejecución y Pruebas - Módulo de Interfaz (INT)

## 1. Preparar Dependencias

Asegúrate de tener la biblioteca Crow en la carpeta ```include``` antes de la primera compilación:

```bash
cd src/int/include/
wget https://github.com/CrowCpp/Crow/releases/download/v1.0+5/crow_all.h
```

## 2. Configuración de Permisos

Antes de ejecutar los scripts por primera vez, debes otorgarles permisos de ejecución:

```bash
cd src/int/
chmod +x launch_app launch_web
```

## 3. Ejecución del Sistema

El sistema se divide en dos procesos independientes para mayor modularidad:

### A. Servidor Backend (C++)

Ejecuta el script ```launch_app```. Este realizará una limpieza de caché (clear), un Clean de CMake, compilará el proyecto y lanzará el servidor en el puerto 8080:

```bash
./launch_app
```

### B. Interfaz Frontend (Web)

En una nueva terminal, ejecuta ```launch_web```. Esto abrirá automáticamente el navegador en la dirección del servidor local:

```bash
./launch_web
```

## 4. Sistema de Autenticación (Acceso al Robot)

Al abrir la interfaz web, se presentará un modal de **Control de Acceso**. Es obligatorio autenticarse para activar el canal de comunicación WebSocket y habilitar los controles manuales.

### A. Registro de Nuevo Usuario:
1. Haz clic en el enlace **"Regístrate aquí"** dentro del modal.
2. Ingrese un nombre de usuario y una contraseña que cumpla con los requisitos de seguridad: **8+ caracteres, Mayúscula, Minúscula, Número y Símbolo (ej. . o #)**.
3. Tras el éxito, los datos se persistirán físicamente en el servidor en la ruta: `data/r_users/users_robot.json`.

### B. Credenciales de Prueba (Test Credentials):
Para agilizar las pruebas de desarrollo, se puede utilizar el siguiente usuario pre-validado:
* **Usuario:** `test2026`
* **Contraseña:** `Test.2026#`

### C. Inicio de Sesión y Seguridad:
1. Introduzca sus credenciales en el formulario de login.
2. **Protección de Fuerza Bruta**: El sistema bloqueará el acceso al usuario temporalmente tras **3 intentos fallidos**. El mensaje de error indicará el tiempo restante de bloqueo.
3. Una vez logueado con éxito, el modal se cerrará automáticamente y el indicador de estado cambiará a **"Conectado al Robot"**, iniciando el flujo de telemetría.

## 5. Pruebas de Navegación y Obstáculos 

Una vez que el sistema esté corriendo y el indicador de estado en la web marque **"Conectado"**, puedes realizar pruebas de estrés de mapeo:

### A. Generar Obstáculos Aleatorios:
Abre la consola del navegador (**F12**) y ejecuta el siguiente comando para poblar el mapa con obstáculos aleatorios:
```javascript
fetch('/api/test/obstacles')
```

### B. Validación de Colisiones:

Utiliza el control remoto para intentar mover el robot (triángulo cian) hacia los obstáculos rojos. El sistema debería detener el movimiento y emitir una alerta sonora.

### C. Reset del Mapa:

Presiona el botón STOP (⏹) en el control remoto. Esto ejecutará la función resetMap(), limpiando la grilla y regresando al robot a su posición inicial configurada.