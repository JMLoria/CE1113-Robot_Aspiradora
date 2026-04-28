#ifndef MAPPING_H
#define MAPPING_H

#include <vector>
#include <string>
#include <mutex>

enum CellState
{
    UNKNOWN = 0,
    FREE = 1,
    OBSTACLE = 2
};

class MappingManager
{
public:
    MappingManager(int width, int height, int startX = -1, int startY = -1);

    // Actualiza la celda actual del robot y marca la nueva posición como libre.
    void updateRobotPosition(int x, int y);

    // Marca una celda como obstáculo en la representación interna del mapa.
    void addObstacle(int x, int y);

    // Serializa el estado del mapa para consumo del frontend por WebSocket.
    std::string getMapAsJson();

    // Consulta si una coordenada está dentro de límites y no está bloqueada.
    bool isTraversable(int x, int y);

    // Limpia el mapa y reposiciona el robot en la celda de inicio.
    void resetMap(int startX = -1, int startY = -1);

private:
    int width, height;
    int robotX, robotY;
    int robotAngle = 0; // Referencia visual del robot: 0=arriba, 90=derecha, 180=abajo, 270=izquierda.
    std::vector<std::vector<int>> grid;
    std::mutex mapMutex; // Protege el mapa frente a accesos concurrentes desde HTTP y WebSocket.
};

#endif // MAPPING_H