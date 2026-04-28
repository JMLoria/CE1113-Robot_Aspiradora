#include "../include/mapping.h"
#include <sstream>
#include <iostream>

MappingManager::MappingManager(int w, int h, int startX, int startY) : width(w), height(h)
{
    grid.resize(height, std::vector<int>(width, UNKNOWN));

    // Si no se especifica una posición inicial, el robot parte desde el centro del mapa.
    robotX = (startX == -1) ? width / 2 : startX;
    robotY = (startY == -1) ? height / 2 : startY;

    if (robotX >= 0 && robotX < width && robotY >= 0 && robotY < height)
    {
        // La celda de inicio se marca como libre para reflejar la posición inicial del robot.
        grid[robotY][robotY] = FREE;
    }
}

void MappingManager::updateRobotPosition(int x, int y)
{
    std::lock_guard<std::mutex> lock(mapMutex);
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        robotX = x;
        robotY = y;
        grid[robotY][robotX] = FREE;
    }
}

void MappingManager::addObstacle(int x, int y)
{
    std::lock_guard<std::mutex> lock(mapMutex);
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        // El mapa conserva obstáculos conocidos aunque el robot ya no esté cerca de esa celda.
        grid[y][x] = OBSTACLE;
    }
}

std::string MappingManager::getMapAsJson()
{
    std::lock_guard<std::mutex> lock(mapMutex);
    std::stringstream ss;
    // El formato JSON se mantiene compacto para minimizar el costo de envío por WebSocket.
    ss << "{\"robot\": [" << robotX << ", " << robotY << "], \"angle\": " << robotAngle << ", \"grid\": [";
    for (int i = 0; i < height; ++i)
    {
        ss << "[";
        for (int j = 0; j < width; ++j)
        {
            ss << grid[i][j] << (j == width - 1 ? "" : ",");
        }
        ss << "]" << (i == height - 1 ? "" : ",");
    }
    ss << "]}";
    return ss.str();
}

bool MappingManager::isTraversable(int x, int y)
{
    std::lock_guard<std::mutex> lock(mapMutex);
    // La validación de límites evita accesos fuera de rango antes de consultar la celda.
    if (x < 0 || x >= width || y < 0 || y >= height)
        return false;
    // Solo una celda libre se considera transitable para la planificación de movimiento.
    return grid[y][x] != OBSTACLE;
}

void MappingManager::resetMap(int startX, int startY)
{
    std::lock_guard<std::mutex> lock(mapMutex);

    // Reinicio total: se descartan celdas conocidas y se vuelve al estado base.
    for (auto &row : grid)
    {
        std::fill(row.begin(), row.end(), UNKNOWN);
    }

    // Se recalcula el punto de arranque usando la misma convención del constructor.
    robotX = (startX == -1) ? width / 2 : startX;
    robotY = (startY == -1) ? height / 2 : startY;

    if (robotX >= 0 && robotX < width && robotY >= 0 && robotY < height)
    {
        grid[robotY][robotY] = FREE;
    }

    std::cout << "[MAPA] Matriz reiniciada." << std::endl;
}