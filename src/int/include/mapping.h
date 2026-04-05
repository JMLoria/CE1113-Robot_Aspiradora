#ifndef MAPPING_H
#define MAPPING_H

#include <vector>
#include <string>
#include <mutex>

enum CellState { UNKNOWN = 0, FREE = 1, OBSTACLE = 2};

class MappingManager {
public:
    MappingManager(int width, int height);

    // Actualiza la posicion del robot y marca como "FREE"
    void updateRobotPosition(int x, int y);

    // Registra un obstaculo detectado
    void addObstacle(int x, int y);

    // Convierte la grilla a JSON para enviarla por el WebSocket de Crow
    std::string getMapAsJson();

    void resetMap();

private:
    int width, height;
    int robotX, robotY;
    int robotAngle = 0;     // 0: Arriba, 90: Derecha, 180: Abajo, 270: Izquierda
    std::vector<std::vector<int>> grid;
    std::mutex mapMutex;    // Para evitar problemas con los hilos del servidor
};

#endif // MAPPING_H