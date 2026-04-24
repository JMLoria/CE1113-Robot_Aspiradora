#include "../include/crow_all.h"
#include "../include/audio_manager.h"
#include "../include/mapping.h"
#include "../include/user_manager.h"
#include "../include/biblioteca_robot.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <random>


int main() {
    crow::SimpleApp app;

    UserManager auth("../../../data/r_users/users_robot.json");

    AudioManager audio; 
    MappingManager mapper(20, 20);

    // Estado local para el control de orientacion
    int robot_angle = 0;
    // Inicia en el centro de la grilla 
    int cur_x = 10;
    int cur_y = 10;
    // Variables de estado LEDs
    bool is_autonomous = false;
    bool obstacle_alert = false;
    bool system_on = true;

    // Alerta de encendido al iniciar el servidor
    audio.notifications("sys_on");
    
    // --- ARCHIVOS ESTATICOS ---
    // Sirve el index.html automaticamente al entrar a http://localhost:8080
    CROW_ROUTE(app, "/")    // Ruta para el HTML
    ([]() {
        std::ifstream f("../web/index.html");
        if (f) {
            std::stringstream buffer;
            buffer << f.rdbuf();
            crow::response res(buffer.str());

            res.set_header("Content-Type", "text/html; charset=UTF-8");
            res.set_header("Access-Control-Allow-Origin", "*");

            return res;
        }
        return crow::response(404, "index.html no encontrado");
    });

    CROW_ROUTE(app, "/app.js")  // Ruta para el JS de app
    ([]() {
        std::ifstream f("../web/app.js");
        if (f) {
            std::stringstream buffer;
            buffer << f.rdbuf();
            crow::response res(buffer.str());

            res.set_header("Content-Type", "application/javascript");
            res.set_header("Access-Control-Allow-Origin", "*");
            
            return res;
        }
        return crow::response(404);
    });

    CROW_ROUTE(app, "/auth.js") // Ruta para el JS de auth
    ([]() {
        std::ifstream f("../web/auth.js");
        if (f) {
            std::stringstream buffer;
            buffer << f.rdbuf();
            crow::response res(buffer.str());
            res.set_header("Content-Type", "application/javascript");
            return res;
        }
        return crow::response(404, "auth.js no encontrado");
    });

    CROW_ROUTE(app, "/style.css") // Ruta para el CSS
    ([]() {
        std::ifstream f("../web/style.css");
        if (f) {
            std::stringstream buffer;
            buffer << f.rdbuf();
            crow::response res(buffer.str());

            res.set_header("Content-Type", "text/css");
            res.set_header("Access-Control-Allow-Origin", "*");
            return res;
        }
        return crow::response(404);
    });

    // --- ENPOINTS DE ESTADO ---
    CROW_ROUTE(app, "/api/mode/<string>")
    ([&](const crow::request& req, std::string mode) {
        is_autonomous = (mode == "auto"); 
        std::cout << "[MODO] Cambiando a: " << mode << "[" << is_autonomous << "]" << std::endl;
        crow::response res(200, "Modo actualizado");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Modo actualizado");
    });

    // --- ENDPOINTS DE AUTENTICACION ---
    CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::POST) // Ruta de register
    ([&auth](const crow::request& req) {
        auto x = crow::json::load(req.body);
        if (!x) return crow::response(400, "{\"message\":\"JSON Inválido\"}");

        std::string user = x["username"].s();
        std::string pass = x["password"].s();

        crow::json::wvalue res;
        if (auth.registerUser(user, pass)) {
            res["status"] = "success";
            res["message"] = "Usuario creado exitosamente";
            return crow::response(201, res);
        } else {
            res["status"] = "error";
            res["message"] = "El usuario ya existe o no es válido";
            return crow::response(400, res);
        }
    });

    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST) // Ruta de login
    ([&auth](const crow::request& req) {
        auto x = crow::json::load(req.body);
        if (!x) return crow::response(400, "{\"message\":\"JSON Inválido\"}");

        std::string user = x["username"].s();
        std::string pass = x["password"].s();

        crow::json::wvalue res;
        if (auth.authenticate(user, pass)) {
            res["status"] = "success";
            res["message"] = "Login correcto";
            return crow::response(200, res);
        } else {
            res["status"] = "error";
            if (auth.isLocked(user)) {
                res["message"] = "Bloqueado. Intente en " + std::to_string(auth.getRemainingLockTime(user)) + "s";
            } else {
                res["message"] = "Credenciales incorrectas";
            }
            return crow::response(401, res);
        }
    });

    // --- ENDPOINTS CONTROL REMOTO MANUAL ---  
    CROW_ROUTE(app, "/api/move/<string>") 
    ([&](const crow::request& req, std::string dir) {
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
            mapper.resetMap();

            cur_x = 10;
            cur_y = 10;
            robot_angle = 0;

            std::cout << "[SISTEMA] Stop: Motores detenidos y mapa limpio" << std::endl;
        }
        crow::response res(200, "OK");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "OK");
    });

    // --- ENDPONTS MUSICA ---
    CROW_ROUTE(app, "/api/audio/play/current")
    ([&](const crow::request& req) {
        audio.play();
        crow::response res(200, "Reproduciondo cancion");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Reproduciondo cancion");
    });

    CROW_ROUTE(app, "/api/audio/play/<int>")
    ([&](const crow::request& req, int track_id) {
        audio.play(track_id);
        crow::response res(200, "Reproduciondo cancion");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Reproduciondo cancion");
    });

    CROW_ROUTE(app, "/api/audio/playlist")
    ([&]() {
        std::vector<std::string> songs = audio.getPlaylist(); 

        crow::json::wvalue response;
        response = songs;

        crow::response res(response);
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        return res;
    });

    CROW_ROUTE(app, "/api/audio/play_specific")
    .methods("POST"_method)
    ([&](const crow::request& req) {
        auto song_name = req.url_params.get("name");
        if (song_name) {
            audio.playSpecific(song_name);

            crow::response res(200, "Reproduciendo: " + std::string(song_name));
            res.set_header("Access-Control-Allow-Origin", "*");
            return res;
        }
        crow::response error_res(400, "Nombre no proporcionado");
        error_res.set_header("Access-Control-Allow-Origin", "*");
        return error_res;
    });

    CROW_ROUTE(app, "/api/audio/pause")
    ([&](const crow::request& req) {
        audio.pause();
        crow::response res(200, "Pausa/Reanudar");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Pausa/Reanudar");
    });

    CROW_ROUTE(app, "/api/audio/stop")
    ([&](const crow::request& req) {
        audio.stop();
        crow::response res(200, "Reproduccion detenida");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Reproduccion detenida");
    });

    CROW_ROUTE(app, "/api/audio/next")
    ([&](const crow::request& req) {
        audio.nextSong();
        crow::response res(200, "Siguiente");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Siguiente");
    });

    CROW_ROUTE(app, "/api/audio/prev")
    ([&](const crow::request& req) {
        audio.prevSong();
        crow::response res(200, "Anterior");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Anterior");
    });

    // --- ENDPOINTS CONTROL AUDIO ---
    CROW_ROUTE(app, "/api/audio/forward")
    ([&](const crow::request& req) {
        audio.forward5s();
        crow::response res(200, "+5s");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "+5s");
    });

    CROW_ROUTE(app, "/api/audio/back")
    ([&](const crow::request& req) {
        audio.back5s();
        crow::response res(200, "-5s");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "-5s");
    });

    CROW_ROUTE(app, "/api/audio/volume/up")
    ([&](const crow::request& req) {
        audio.upVolume();
        crow::response res(200, "Volumen +");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Volumen +");
    });

    CROW_ROUTE(app, "/api/audio/volume/down")
    ([&](const crow::request& req) {
        audio.downVolume();
        crow::response res(200, "Volumen -");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Volumen -");
    });

    // --- ENDPOINTS NOTIFICACIONES (SISTEMA) ---
    CROW_ROUTE(app, "/api/audio/notify/<string>")
    ([&](const crow::request& req, std::string alert_name) {
        audio.notifications(alert_name);
        crow::response res(200, "Notificacion enviada");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Notificacion enviada");
    });

    // --- ENDPOINT DE PRUEBA
    CROW_ROUTE(app, "/api/test/obstacles")
    ([&](const crow::request& req) {
        std::cout << "[TEST] Generando obstáculos de prueba..." << std::endl;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> disX(0, 19);
        std::uniform_int_distribution<> disY(0, 19);

        // Genera N obstaculos al azar
        for (int i = 0; i < 35; i++) {
            int obsX = disX(gen);
            int obsY = disY(gen);

            if (obsX != cur_x || obsY != cur_y) {
                mapper.addObstacle(obsX, obsY);
            }
        }

        crow::response res(200, "Obstaculos generados.");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Obstaculos generados.");
    });

    // --- WEBSOCKET ---
    CROW_ROUTE(app, "/ws")
    .websocket()
    .onopen([&](crow::websocket::connection& conn) {
        std::cout << "[WS] Cliente conectado. Sincronizando..." << std::endl;
    })
    .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
        if (data == "update") {
            // Crea un objeto JSON de respuesta
            crow::json::wvalue response;
            
            // 1. Datos del Mapa (Parsea el JSON que genera el mapper)
            auto map_data = crow::json::load(mapper.getMapAsJson());
            response["map"]["grid"] = map_data["grid"];
            response["map"]["robot"] = map_data["robot"];
            response["map"]["angle"] = robot_angle;

            // 2. Datos de Audio Real (Extraídos del hilo de mpg123)
            response["audio"]["track"] = audio.getCurrentTrackName();
            response["audio"]["current"] = audio.getCurrentTime();
            response["audio"]["total"] = audio.getTotalTime();
            response["audio"]["volume"] = audio.getVolume();

            // 3. Estado de LEDs y Modos
            response["status"]["autonomous"] = is_autonomous;
            response["status"]["manual"] = !is_autonomous;
            response["status"]["obstacle"] = obstacle_alert;
            response["status"]["system"] = system_on;
            
            // Enviamos todo el paquete al navegador
            conn.send_text(response.dump());
        }
    });

    std::cout << "\n==========================================" << std::endl;
    std::cout << "SERVIDOR INICADO" << std::endl;
    std::cout << "Abrir en navegador: http://localhost:8080" << std::endl;
    std::cout << "==========================================\n" << std::endl;

    app.loglevel(crow::LogLevel::Warning); // Esto ocultará los "Request/Response" constantes y solo mostrará errores
    app.port(8080).multithreaded().run();
    return 0;
}