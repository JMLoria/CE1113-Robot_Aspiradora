cmake_minimum_required(VERSION 3.10)
project(RobotAspiradora)

set(CMAKE_CXX_STANDARD 17)

# Incluir cabeceras
include_directories(include)

set(SOURCES
    app/main_int.cpp
    app/audio_manager.cpp
    app/mapping.cpp
    app/user_manager.cpp
)

# Crear el ejecutable
add_executable(robot_int_serv ${SOURCES})

# Hilos necesarios para Crow y audio concurrente
find_package(Threads REQUIRED)
target_link_libraries(robot_int_serv PRIVATE Threads::Threads)

# Nota: Cuando el ROL-OS tenga lista la librearía .so, se vincula aquí:
# target_link_libraries(robot_int_serv PRIVATE ${CMAKE_SOURCE_DIR}/lib/librobot.so)