cmake_minimum_required(VERSION 3.10)
project(RobotAspiradora_INT LANGUAGES C CXX) # Forzamos C y C++

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. BUSCAR DEPENDENCIAS (Esto requiere que en tu .bb tengas DEPENDS += "boost")
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

# 2. INCLUIR CABECERAS
# Usamos CMAKE_CURRENT_SOURCE_DIR para no perdernos
include_directories(
    include 
    ${CMAKE_CURRENT_SOURCE_DIR}/../librobot/include
)

# 3. FUENTES (Asegúrate de que la carpeta 'app' esté dentro de 'int')
set(INT_SOURCES
    app/main_int.cpp
    app/audio_manager.cpp
    app/mapping.cpp
    app/user_manager.cpp
)

# 4. CREAR EJECUTABLE
add_executable(robot_int_serv ${INT_SOURCES})

# 5. VINCULACIÓN
target_link_libraries(robot_int_serv 
    PRIVATE 
    robot          # CMake buscará 'librobot.so' automáticamente
    Boost::system
    Boost::thread
    Threads::Threads
    crypt
)