#include "../include/mapping.h"
#include <sstream>
#include <iostream>

MappingManager::MappingManager(int w, int h, int startX, int startY) : width(w), height(h) {
    grid.resize(height, std::vector<int>(width, UNKNOWN));

    // Si la posicion inicial no se especifica, va al centro. Sis e especifica, usa ese valor
    robotX = (startX == -1) ? width / 2 : startX;
    robotY = (startY == -1) ? height / 2 : startY;
    
    if (robotX >= 0 && robotX < width && robotY >= 0 && robotY < height) {
        grid[robotY][robotY] = FREE;
    }
}

void MappingManager::updateRobotPosition(int x, int y) {
    std::lock_guard<std::mutex> lock(mapMutex);
    if (x >= 0 && x < width && y >= 0 && y < height) {
        robotX = x;
        robotY = y;
        grid[robotY][robotX] = FREE;
    }
}

void MappingManager::addObstacle(int x, int y) {
    std::lock_guard<std::mutex> lock(mapMutex);
    if (x >= 0 && x < width && y >= 0 && y < height) {
        grid[y][x] = OBSTACLE;
    }
}

std::string MappingManager::getMapAsJson() {
    std::lock_guard<std::mutex> lock(mapMutex);
    std::stringstream ss;
    ss << "{\"robot\": [" << robotX << ", " << robotY << "], \"angle\": " << robotAngle << ", \"grid\": [";
    for (int i = 0; i < height; ++i) {
        ss << "[";
        for (int j = 0; j < width; ++j) {
            ss << grid[i][j] << (j == width -1 ? "" : ",");
        }
        ss << "]" << (i == height - 1 ? "" : ",");
    }
    ss << "]}";
    return ss.str();
}

bool MappingManager::isTraversable(int x, int y) {
    std::lock_guard<std::mutex> lock(mapMutex);
    // Verificar limites del mapa
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    // Retorna true solo si NO es un obstaculo
    return grid[y][x] != OBSTACLE;
}


void MappingManager::resetMap(int startX, int startY) {
    std::lock_guard<std::mutex> lock(mapMutex);

    // Llenar la matriz con UNKNOWN
    for (auto& row : grid) {
        std::fill(row.begin(), row.end(), UNKNOWN);
    }

    // Reposicionar al robot
    robotX = (startX == -1) ? width / 2 : startX;
    robotY = (startY == -1) ? height / 2 : startY;

    if (robotX >= 0 && robotX < width && robotY >= 0 && robotY < height) {
        grid[robotY][robotY] = FREE;
    }

    std::cout << "[MAPA] Matriz reiniciada." << std::endl;
}