# Guía de Ejecución y Pruebas

## 1. Preparar carpetas y descargar dependencias
Dentro de la carpeta include es decir:
```bash
cd src/int/include/
wget https://github.com/CrowCpp/Crow/releases/download/v1.0+5/crow_all.h
```

## 2. Compilar
```bash
cd src/int/
mkdir build && cd build
cmake ..
make
```

## 3. Ejecutar
```bash
./robot_int_serv
```