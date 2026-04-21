cmake_minimum_required(VERSION 3.16)
project(librobot VERSION 1.0.0 LANGUAGES C)

# Estándar C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Archivos fuente
set(SOURCES
    src/robot.c
    src/motors.c
    src/sensors.c
    src/leds.c
    src/audio.c
)

# Biblioteca dinámica
add_library(robot SHARED ${SOURCES})

# Headers públicos
target_include_directories(robot PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Dependencias externas
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)
pkg_check_modules(ALSA REQUIRED alsa)
pkg_check_modules(MPG123 REQUIRED libmpg123)

target_link_libraries(robot
    ${GPIOD_LIBRARIES}
    ${ALSA_LIBRARIES}
    ${MPG123_LIBRARIES}
)

target_include_directories(robot PRIVATE
    ${GPIOD_INCLUDE_DIRS}
    ${ALSA_INCLUDE_DIRS}
    ${MPG123_INCLUDE_DIRS}
)

# Instalación
include(GNUInstallDirs)
install(TARGETS robot
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES include/librobot.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

add_executable(test_hardware test_hardware.c)
target_link_libraries(test_hardware robot)