#include "../include/crow_all.h"
#include "../include/audio_manager.h"
#include "../include/mapping.h"
#include "../include/biblioteca_robot.h"
#include <iostream>


int main() {
    crow::SimpleApp app;
    AudioManager audio; 
    MappingManager mapper(20, 20);
    
    // --- Ruta para Achivos Estaticos ---
    // Sirve el index.html automaticamente al entrar a http://localhost:8080
    CROW_ROUTE(app, "/")
    ([](const crow::request& req, crow::response& res) {
        std::string path = "/home/jose/Documents/TEC/2026/I_Semestre/CE1113-Empotrados/CE1113-Robot_Aspiradora/src/int/web/index.html";
    
        std::ifstream f(path);
        if (f.is_open()) {
            std::stringstream buffer;
            buffer << f.rdbuf();
            res.set_header("Content-Type", "text/html");
            res.write(buffer.str());
            std::cout << "[SUCCESS] Archivo index.html leido correctamente." << std::endl;
        } else {
            res.code = 404;
            res.write("Error critico: El servidor no puede abrir el archivo en la ruta especificada.");
            std::cout << "[ERROR] No se pudo abrir el archivo en: " << path << std::endl;
        }
        res.end();
    });

    // Ruta para el JavaScript (Lectura manual)
    CROW_ROUTE(app, "/app.js")
    ([](const crow::request& req, crow::response& res) {
        std::string path = "/home/jose/Documents/TEC/2026/I_Semestre/CE1113-Empotrados/CE1113-Robot_Aspiradora/src/int/web/app.js";
        std::ifstream f(path);
        if (f.is_open()) {
            std::stringstream buffer;
            buffer << f.rdbuf();
            res.set_header("Content-Type", "application/javascript");
            res.write(buffer.str());
            std::cout << "[SUCCESS] app.js enviado." << std::endl;
        } else {
            res.code = 404;
            std::cout << "[ERROR] No se encontro app.js en la ruta absoluta." << std::endl;
        }
        res.end();
    });

    // --- Endpoints de prueba ---
    CROW_ROUTE(app, "/api/move/<string>")
    ([&](std::string dir) {
        std::cout << "[CONTROL] Comando recibido: " << dir << std::endl;
        // set_motors(50, 50); // Aqui llamamos a la .so
        return crow::response(200, "OK");
    });

    CROW_ROUTE(app, "/api/audio/play/<string>")
    ([&](std::string track) {
        std::cout << "[AUDIO] Reproduciendo track: " << track << std::endl;
        audio.play(track + ".mp3");
        return crow::response(200, "OK");
    });

    CROW_ROUTE(app, "/api/mode/<string>")
    ([&](std::string mode) {
        std::cout << "[MODO] Cambiando a: " << mode << std::endl;
        return crow::response(200, "Modo actualizado");
    });

    // --- Websocket para el mapa ---
    CROW_ROUTE(app, "/ws")
        .websocket()
        .onopen([&](crow::websocket::connection& conn) {
            std::cout << "[WS] Cliente conectado. Iniciando flujo de datos del mapa..." << std::endl;
            
            // Simulación: Cada vez que se conecta un cliente, le enviamos un mapa inicial
            // En una implementación real, usarías un timer para enviar esto constantemente
            conn.send_text(mapper.getMapAsJson());
        })
        .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
            // Si el cliente pide una actualización manual enviando "update"
            if (data == "update") {
                // Simulamos que el robot se movió un poco antes de enviar
                static int move_x = 10;
                mapper.updateRobotPosition(move_x++, 10);
                
                // Enviamos el mapa actualizado
                conn.send_text(mapper.getMapAsJson());
                std::cout << "[WS] Mapa actualizado enviado a petición del cliente" << std::endl;
            }
        });

    std::cout << "\n==========================================" << std::endl;
    std::cout << "SERVIDOR INICADO" << std::endl;
    std::cout << "Abrir en navegador: http://localhost:8080" << std::endl;
    std::cout << "==========================================\n" << std::endl;

    app.port(8080).multithreaded().run();
    return 0;
}