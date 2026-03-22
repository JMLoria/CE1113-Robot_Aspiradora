#include "../include/mapping.h"
#include <sstream>

MappingManager::MappingManager(int w, int h) : width(w), height(h) {
    grid.resize(height, std::vector<int>(width, UNKNOWN));
    robotX = width / 2;
    robotY = height / 2;
}

void MappingManager::updateRobotPosition(int x, int y) {
    std::lock_guard<std::mutex> lock(mapMutex);
    if (x >= 0 && x < width && y >= 0 && y < height) {
        robotX = x;
        robotY = y;
        grid[y][x] = FREE;
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
    ss << "{\"robot\": [" << robotX << ", " << robotY << "], \"grid\": [";
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