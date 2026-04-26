cmake_minimum_required(VERSION 3.10)
project(RobotAspiradora_Principal)

# 1. Incluir la librería de hardware
add_subdirectory(os/librobot)

# 2. Incluir el servidor de integración
add_subdirectory(int)