cmake_minimum_required(VERSION 3.10)
project(RobotAspiradora)

set(CMAKE_CXX_STANDARD 17)

# 1. Incluir cabeceras
include_directories(include)

# 2. Fuentes del proyecto
set(SOURCES
    app/main_int.cpp
    app/audio_manager.cpp
    app/mapping.cpp
    app/user_manager.cpp
)

# 3. Crear el ejecutable
add_executable(robot_int_serv ${SOURCES})

# 4. Vinculacion de Librerias
find_package(Threads REQUIRED)

target_link_libraries(robot_int_serv 
    PRIVATE 
    Threads::Threads
    crypt
)

# Nota: Cuando el ROL-OS tenga lista la librearía .so, se vincula aquí:
# target_link_libraries(robot_int_serv PRIVATE ${CMAKE_SOURCE_DIR}/lib/librobot.so)