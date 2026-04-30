cmake_minimum_required(VERSION 3.10)
project(RobotAspiradoraMaster)

# Le decimos a CMake que compile ambos subproyectos que están en files/
add_subdirectory(librobot)
add_subdirectory(int)     

include_directories(
    librobot/include
    int/include
)
