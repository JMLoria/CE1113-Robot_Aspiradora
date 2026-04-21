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

## 4. Pruebas de Navegación y Obstáculos 

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