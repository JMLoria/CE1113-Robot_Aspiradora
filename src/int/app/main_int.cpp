#include "../include/crow_all.h"
#include "../include/audio_manager.h"
#include "../include/mapping.h"
#include "../include/biblioteca_robot.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>


int main() {
    crow::SimpleApp app;
    AudioManager audio; 
    MappingManager mapper(20, 20);

    // Estado local para el control de orientacion
    int robot_angle = 0;
    // Inicia en el centro de la grilla 
    int cur_x = 10;
    int cur_y = 10;

    // Alerta de encendido al iniciar el servidor
    audio.notifications("sys_on");
    
    // --- ARCHIVOS ESTATICOS ---
    // Sirve el index.html automaticamente al entrar a http://localhost:8080
    CROW_ROUTE(app, "/")
    ([](const crow::request& req, crow::response& res) {
        std::string path = "/home/jose/Documents/TEC/2026/I_Semestre/CE1113-Empotrados/CE1113-Robot_Aspiradora/src/int/web/index.html";
        std::ifstream f(path);
        if (f.is_open()) {
            std::stringstream buffer;
            buffer << f.rdbuf();
            res.set_header("Content-Type", "text/html; charset=UTF-8");
            res.write(buffer.str());
            std::cout << "[SUCCESS] index.html enviado correctamente." << std::endl;
        } else {
            res.code = 404;
            res.write("Error critico: No se encontro index.html");
        }
        res.end();
    });

    CROW_ROUTE(app, "/app.js")
    ([](const crow::request& req, crow::response& res) {
        std::string path = "/home/jose/Documents/TEC/2026/I_Semestre/CE1113-Empotrados/CE1113-Robot_Aspiradora/src/int/web/app.js";
        std::ifstream f(path);
        if (f.is_open()) {
            std::stringstream buffer;
            buffer << f.rdbuf();
            res.set_header("Content-Type", "application/javascript");
            res.write(buffer.str());
            std::cout << "[SUCCESS] app.js enviado correctamente." << std::endl;
        } else {
            res.code = 404;
        }
        res.end();
    });

    CROW_ROUTE(app, "/api/mode/<string>")
    ([&](std::string mode) {
        std::cout << "[MODO] Cambiando a: " << mode << std::endl;
        return crow::response(200, "Modo actualizado");
    });

    // --- ENDPOINTS CONTROL REMOTO MANUAL ---  
    CROW_ROUTE(app, "/api/move/<string>") 
    ([&](std::string dir) {
        if (dir == "left") {
            robot_angle = (robot_angle - 90 + 360) % 360;
            std::cout << "[MOTOR] Rotando Izquierda. Nuevo angulo: " << robot_angle << std::endl;
        } else if (dir == "right") {
            robot_angle = (robot_angle + 90) % 360;
            std::cout << "[MOTOR] Rotando Derecha. Nuevo angulo: " << robot_angle << std::endl;
        } else if (dir == "forward" || dir == "backward") {
            int step = (dir == "forward") ? 1 : -1;
            int next_x = cur_x;
            int next_y = cur_y;

            // Logica de movimiento segun orientacion
            if (robot_angle == 0)   next_y -= step;  // Norte
            if (robot_angle == 90)  next_x += step;  // Este
            if (robot_angle == 180) next_y += step;  // Sur
            if (robot_angle == 270) next_x -= step;  // Oeste

            if (mapper.isTraversable(next_x, next_y)) {
                cur_x = next_x;
                cur_y = next_y;
                mapper.updateRobotPosition(cur_x, cur_y);
                std::cout << "[MOTOR] " << dir << std::endl;
            } else {
                std::cout << "[ALERTA] Obstáculo detectado en " << next_x << "," << next_y << ". Movimiento cancelado." << std::endl;
                audio.notifications("obstacle");
            }
        } else if (dir == "stop") {
            std::cout << "[MOTOR] Detenido" << std::endl;
        }
        return crow::response(200, "OK");
    });

    // --- ENDPONTS MUSICA ---
    CROW_ROUTE(app, "/api/audio/play/current")
    ([&]() {
        audio.play();
        return crow::response(200, "Reproduciondo cancion");
    });

    CROW_ROUTE(app, "/api/audio/play/<int>")
    ([&](int track_id) {
        audio.play(track_id);
        return crow::response(200, "Reproduciondo cancion");
    });

    CROW_ROUTE(app, "/api/audio/pause")
    ([&]() {
        audio.pause();
        return crow::response(200, "Pausa/Reanudar");
    });

    CROW_ROUTE(app, "/api/audio/stop")
    ([&]() {
        audio.stop();
        return crow::response(200, "Reproduccion detenida");
    });

    CROW_ROUTE(app, "/api/audio/next")
    ([&]() {
        audio.nextSong();
        return crow::response(200, "Siguiente");
    });

    CROW_ROUTE(app, "/api/audio/prev")
    ([&]() {
        audio.prevSong();
        return crow::response(200, "Anterior");
    });

    // --- ENDPOINTS CONTROL AUDIO ---
    CROW_ROUTE(app, "/api/audio/forward")
    ([&]() {
        audio.forward5s();
        return crow::response(200, "+5s");
    });

    CROW_ROUTE(app, "/api/audio/back")
    ([&]() {
        audio.back5s();
        return crow::response(200, "-5s");
    });

    CROW_ROUTE(app, "/api/audio/volume/up")
    ([&]() {
        audio.upVolume();
        return crow::response(200, "Volumen +");
    });

    CROW_ROUTE(app, "/api/audio/volume/down")
    ([&]() {
        audio.downVolume();
        return crow::response(200, "Volumen -");
    });

    // --- ENDPOINTS NOTIFICACIONES (SISTEMA) ---
    CROW_ROUTE(app, "/api/audio/notify/<string>")
    ([&](std::string alert_name) {
        audio.notifications(alert_name);
        return crow::response(200, "Notificacion enviada");
    });

    // --- ENDPOINT DE PRUEBA
    CROW_ROUTE(app, "/api/test/obstacles")
    ([&]() {
        std::cout << "[TEST] Generando obstáculos de prueba..." << std::endl;
        
        // Dibujamos una pequeña "pared" o borde para probar
        for(int i = 5; i < 15; i++) {
            mapper.addObstacle(i, 5);  // Línea horizontal
            mapper.addObstacle(5, i);  // Línea vertical
        }
        
        // Un obstáculo aleatorio cerca del robot
        mapper.addObstacle(12, 12);
        mapper.addObstacle(8, 8);

        return crow::response(200, "Obstaculos generados. Revisa la interfaz web.");
    });

    // --- WEBSOCKET ---
    CROW_ROUTE(app, "/ws")
    .websocket()
    .onopen([&](crow::websocket::connection& conn) {
        std::cout << "[WS] Cliente conectado. Sincronizando..." << std::endl;
    })
    .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
        if (data == "update") {
            // Creamos un objeto JSON de respuesta
            crow::json::wvalue response;
            
            // 1. Datos del Mapa (Parseamos el JSON que genera el mapper)
            auto map_data = crow::json::load(mapper.getMapAsJson());
            response["map"]["grid"] = map_data["grid"];
            response["map"]["robot"] = map_data["robot"];
            response["map"]["angle"] = robot_angle;

            // 2. Datos de Audio Real (Extraídos del hilo de mpg123)
            response["audio"]["track"] = audio.getCurrentTrackName();
            response["audio"]["current"] = audio.getCurrentTime();
            response["audio"]["total"] = audio.getTotalTime();
            
            // Enviamos todo el paquete al navegador
            conn.send_text(response.dump());
        }
    });

    std::cout << "\n==========================================" << std::endl;
    std::cout << "SERVIDOR INICADO" << std::endl;
    std::cout << "Abrir en navegador: http://localhost:8080" << std::endl;
    std::cout << "==========================================\n" << std::endl;

    app.port(8080).multithreaded().run();
    return 0;
}