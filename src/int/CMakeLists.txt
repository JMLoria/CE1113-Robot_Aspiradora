cmake_minimum_required(VERSION 3.10)
project(RobotAspiradora)

set(CMAKE_CXX_STANDARD 17)

# Incluir cabeceras para el proyecto de integración:
include_directories(
    include 
    ${CMAKE_CURRENT_SOURCE_DIR}/../librobot/include
)

# Y para la librería:
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../librobot/build_rpi)
set(SOURCEScmake_minimum_required(VERSION 3.10)
project(RobotAspiradora_INT)

set(CMAKE_CXX_STANDARD 17)

# 1. BUSCAR DEPENDENCIAS
find_package(Threads REQUIRED)
# Buscamos Boost (necesario para Crow)
find_package(Boost REQUIRED COMPONENTS system thread)

# 2. INCLUIR CABECERAS
include_directories(
    include 
    ${CMAKE_CURRENT_SOURCE_DIR}/../librobot/include
)

# 3. FUENTES
set(SOURCES
    app/main_int.cpp
    app/audio_manager.cpp
    app/mapping.cpp
)

# 4. CREAR EJECUTABLE
add_executable(robot_int_serv ${SOURCES})

# 5. VINCULACIÓN MAESTRA (Aquí es donde se arregla el "undefined reference")
target_link_libraries(robot_int_serv 
    PRIVATE 
    librobot          # Tu librería de hardware (OS)
    Boost::boost      # Cabeceras de Boost
    Boost::system     # Binarios de Boost
    Boost::thread
    Threads::Threads  # Hilos
)
    app/main_int.cpp
    app/audio_manager.cpp
    app/mapping.cpp
)

