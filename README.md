# Proyecto I: Robot Aspiradora Autónomo con Yocto y Control Remoto

## 1. Información General
* **Institución:** Instituto Tecnológico de Costa Rica
* **Escuela:** Ingeniería en Computadores
* **Curso:** CE-1113 Sistemas Empotrados
* **Profesor:** Dr.-Ing. Jeferson González Gómez
* **Fecha de entrega:** 28 de abril de 2026
* **Equipo:** Roy Chavarria, Jose Loria, Jose Solano

## 2. Descripción del Proyecto
Desarrollo de un sistema embebido a medida para un robot aspiradora autónomo basado en la **Raspberry Pi 4**. El proyecto utiliza **Yocto Project** para crear una distribución Linux mínima y personalizada que integra navegación reactiva, reproducción de audio MP3 concurrente y una interfaz de control remoto (Web/App) con visualización de mapa 2D.

---

## 3. Estructura Organizativa (Roles)

| Código | Rol | Responsable | Responsabilidades |
| :--- | :--- | :--- | :--- |
| **ROL-OS** | Arquitectura de Sistemas y OS | Roy Chavarria | Configuración de Yocto, recetas BitBake, biblioteca dinámica (.so) y compilación cruzada. |
| **ROL-HW** | Hardware y Control | Jose Solano | Modelo físico, seguridad de potencia, aislamiento galvánico y algoritmos de navegación. |
| **ROL-INT** | Interfaz y Conectividad | Jose Loria | Servidor web, reproducción de audio concurrente, mapeo 2D y autenticación. |

---

## 4. Matriz de Requerimientos

| Código | Categoría | Requerimiento | Descripción Detallada | Responsable |
| :--- | :--- | :--- | :--- | :--- |
| **RF-01** | Funcional | Navegación Autónoma | Implementar un algoritmo reactivo (aleatorio, espiral o zig-zag) para cubrir el área de limpieza. | **ROL-HW** |
| **RF-02** | Funcional | Detección de Obstáculos | Procesar en tiempo real señales de al menos dos sensores (ultrasónicos o infrarrojos) para evitar colisiones. | **ROL-HW** |
| **RF-03** | Funcional | Control de Tracción | Gestionar el movimiento diferencial (avance, retroceso, giros) mediante señales PWM para variar la velocidad. | **ROL-HW** |
| **RF-04** | Funcional | Reproducción MP3 | Reproducir archivos de audio locales de forma concurrente con el movimiento del robot. | **ROL-INT** |
| **RF-05** | Funcional | Retroalimentación Sonora | Emitir audios cortos ante eventos: inicio, detección de obstáculo y cambio de modo (manual/autónomo). | **ROL-INT** |
| **RF-06** | Funcional | Mapeo 2D | Construir una grilla incremental que identifique zonas visitadas, desconocidas y obstáculos detectados. | **ROL-INT** |
| **RF-07** | Funcional | Control Remoto Manual | Permitir el mando directo de los motores desde la interfaz web (flechas de dirección). | **ROL-INT** |
| **RA-01** | Arquitectura y OS | Imagen Yocto Mínima | Construir una distribución Linux optimizada que incluya solo los paquetes necesarios (servidor web, bibliotecas de audio). | **ROL-OS** |
| **RA-02** | Arquitectura y OS | Biblioteca Dinámica | Crear una .so que encapsule el acceso a GPIO (sensores/LEDs), PWM (motores) y audio. | **ROL-OS** |
| **RA-03** | Arquitectura y OS | Receta BitBake (.bb) | Desarrollar una receta propia para integrar el software del proyecto en la imagen de forma automatizada. | **ROL-OS** |
| **RA-04** | Arquitectura y OS | Compilación Cruzada | Utilizar un toolchain ARM y sistema CMake/Autotools para compilar desde el host hacia la Raspberry Pi 4. | **ROL-OS** |
| **RA-05** | Arquitectura y OS | Aislamiento Galvánico | Implementar obligatoriamente optoacopladores entre los pines GPIO y la etapa de potencia de los motores. | **ROL-HW** |
| **RI-01** | Interfaz de Usuario | Panel de Control Web | Visualizar en tiempo real: modo activo, estado de sensores, estado de LEDs y el mapa de recorrido. | **ROL-INT** |
| **RI-02** | Interfaz de Usuario | Gestión de Audio | Controles para seleccionar canciones de una lista, reproducir, pausar, detener y ajustar volumen. | **ROL-INT** |
| **RI-03** | Interfaz de Usuario | Sistema de Login | Pantalla de inicio de sesión segura con al menos un usuario registrado antes de acceder al control. | **ROL-INT** |
| **RI-04** | Interfaz de Usuario | Indicadores Físicos | Gestión de 4 LEDs físicos: Modo Autónomo, Modo Manual, Alerta de Obstáculo y Sistema Encendido. | **ROL-HW** |
| **RNF-01** | No Funcionales | Flujo Git Profesional | Uso de Conventional Commits y ramas (main, develop, feat) para el control de versiones. | **TODOS** |
| **RNF-02** | No Funcionales | Seguridad de Potencia | Uso de reguladores Buck-Boost y BMS para proteger la Raspberry Pi y las celdas Li-Ion. | **ROL-HW** |
| **RNF-03** | No Funcionales | Documentación Técnica | Creación del README con diagramas de arquitectura y manual de compilación cruzada. | **ROL-OS** |
| **RNF-04** | No Funcionales | Atributos Provisionales | Redacción de los documentos de Diseño (DI) y Aprendizaje Continuo (AC) según indicadores del TEC. | **TODOS** |

---

## 5. Flujo de Trabajo en Git

### 5.1. Estrategia de Ramas
* `main`: Rama de producción (código estable).
* `develop`: Rama de integración para el trabajo diario.
* `feat/nombre-tarea`: Desarrollo de nuevas funciones.
* `fix/nombre-error`: Corrección de fallos.

### 5.2. Convención de Commits (Conventional Commits)
Formato: `tipo(alcance): descripción`
* `feat`: Nueva funcionalidad.
* `fix`: Corrección de errores.
* `docs`: Cambios en README o documentación de atributos (DI/AC).
* `refactor`: Mejoras en código existente sin cambiar comportamiento.
* `chore`: Mantenimiento general del proyecto.
* `test`: Creación o actualización de pruebas.
* `ci`: Cambios en flujos de CI/CD.
* `build`: Cambios de compilación, toolchain o sistema de build.
* `perf`: Optimización de rendimiento.
* `style`: Cambios de estilo/formato sin impacto funcional.

### 5.3. Pull Requests
* Todo PR debe estar vinculado a un **Issue**.
* Se requiere al menos **una aprobación** de un compañero para realizar el merge a `develop`.

# Guía de Compilación Cruzada: Robot Aspiradora (CE1113)

Este documento detalla el procedimiento para compilar el código fuente del proyecto **Robot Aspiradora** utilizando el SDK generado por Yocto Project. Este proceso genera binarios compatibles con la arquitectura de la Raspberry Pi 4/5 (Cortex-A72).

##  Procedimiento de Compilación

Sigue estos pasos en el orden indicado para asegurar una construcción limpia y correcta del proyecto.

### 1. Preparar el Entorno (Environment Setup)
Antes de compilar, es necesario exportar las variables de entorno del SDK. Esto asegura que el sistema utilice el compilador cruzado (`cross-compiler`) configurado para la arquitectura destino, en lugar del compilador nativo del equipo.

Abre una terminal y ejecuta el script de entorno:

```bash
source /opt/poky/5.0.17/environment-setup-cortexa76-poky-linux
```

Una vez que el entorno está configurado, se debe navegar al directorio destinado para la compilación (build_rpi) y ejecutar CMake. Esto leerá las instrucciones del proyecto y preparará todo para compilar sin ensuciar el código fuente original.

En caso de que no se haya construido la herramienta anteriormente es necesario ejecutar el comando:

```bash
bitbake -c populate_sdk core-image-minimal
```

Solo es necesario ejectuarlo una vez, si ya se hizo anteriormente no es necesario volverlo a ejecutar.

Una vez que la herramienta fue construida, se debe ejecutar los siguientes comandos en la misma terminal:


### Posicionarse en el directorio de compilación
```bash
cd ~/Embebidos/Proyecto_1/CE1113-Robot_Aspiradora/src/os/librobot/build_rpi
```

### Ejecutar CMake apuntando al directorio superior (donde está el CMakeLists.txt)
```bash
cmake ..
```
