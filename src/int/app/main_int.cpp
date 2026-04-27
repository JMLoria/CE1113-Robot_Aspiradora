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
#include <unordered_set> // Para manejar tokens activos


// Estructura para gestionar sesiones activas
std::unordered_set<std::string> active_tokens;

// Funcion auxiliar para generar un token aleatorio simple
std::string generate_token() {
    static const char alphabet[]= "abcdefghijklmnopqrstuvwxyz0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(alphabet) - 2);
    std::string token = "tk-";
    for (int i = 0; i < 16; ++i) token += alphabet[dis(gen)];
    return token;
}

// Funcion auxiliar para validar tokens en los endpoints
bool is_authorized(const crow::request& req) {
    auto token = req.get_header_value("Authorization");
    if (token.empty()) return false;
    return active_tokens.find(token) != active_tokens.end();
}

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

    // --- ENDPOINTS DE REGISTRO ---
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

    // --- ENDPOINTS DE LOGIN ---
    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST) // Ruta de login
    ([&auth](const crow::request& req) {
        auto x = crow::json::load(req.body);
        if (!x) return crow::response(400, "{\"message\":\"JSON Inválido\"}");

        std::string user = x["username"].s();
        std::string pass = x["password"].s();

        crow::json::wvalue res;
        if (auth.authenticate(user, pass)) {
            std::string new_token = generate_token();
            active_tokens.insert(new_token);

            res["status"] = "success";
            res["token"] = new_token;
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

    // --- ENDPOINTS DE LOGOUT ---
    CROW_ROUTE(app, "/logout").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        auto token = req.get_header_value("Authorization");
        if (!token.empty()) {
            active_tokens.erase(token);
        }
        return crow::response(200, "{\"status\":\"success\"}");
    });

    CROW_ROUTE(app, "/api/<path>")
    .methods(crow::HTTPMethod::OPTIONS)
    ([](const crow::request& req, std::string path) {
        crow::response res(204);
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Authorization, Content-Type");
        return res;
    });
    // --- ENPOINTS DE ESTADO ---
    CROW_ROUTE(app, "/api/mode/<string>")
    ([&](const crow::request& req, std::string mode) {
        if (!is_authorized(req)) {
            return crow::response(403, "Acceso denegado: Token inválido");
        }
        
        is_autonomous = (mode == "auto"); 
        std::cout << "[MODO] Cambiando a: " << mode << "[" << is_autonomous << "]" << std::endl;
        crow::response res(200, "Modo actualizado");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Modo actualizado");
    });

    // --- ENDPOINTS CONTROL REMOTO MANUAL ---  
    CROW_ROUTE(app, "/api/move/<string>") 
    ([&](const crow::request& req, std::string dir) {
        crow::response res;
        // Añadir cabeceras CORS
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Authorization, Content-Type");

        if (!is_authorized(req)) {
            res.code = 403;
            res.body = "{\"status\":\"error\", \"message\":\"No autorizado\"}";
            return res;
        }

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
        res.code = 200;
        res.body = "{\"status\":\"success\"}";
        return res;
    });

    // --- ENDPONTS MUSICA ---
    CROW_ROUTE(app, "/api/audio/play/current")
    ([&](const crow::request& req) {
        if (!is_authorized(req)) {
            return crow::response(403, "Acceso denegado: Token inválido");
        }

        audio.play();
        crow::response res(200, "Reproduciondo cancion");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Reproduciondo cancion");
    });

    CROW_ROUTE(app, "/api/audio/play/<int>")
    ([&](const crow::request& req, int track_id) {
        if (!is_authorized(req)) {
            return crow::response(403, "Acceso denegado: Token inválido");
        }
        
        audio.play(track_id);
        crow::response res(200, "Reproduciondo cancion");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Reproduciondo cancion");
    });

    CROW_ROUTE(app, "/api/audio/playlist")
    ([&](const crow::request& req) {
        if (!is_authorized(req)) {
            return crow::response(403, "Acceso denegado: Token inválido");
        }
        
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
        if (!is_authorized(req)) {
            return crow::response(403, "Acceso denegado: Token inválido");
        }
        
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
        if (!is_authorized(req)) {
            return crow::response(403, "Acceso denegado: Token inválido");
        }
        
        audio.pause();
        crow::response res(200, "Pausa/Reanudar");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Pausa/Reanudar");
    });

    CROW_ROUTE(app, "/api/audio/stop")
    ([&](const crow::request& req) {
        if (!is_authorized(req)) {
            return crow::response(403, "Acceso denegado: Token inválido");
        }
        
        audio.stop();
        crow::response res(200, "Reproduccion detenida");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Reproduccion detenida");
    });

    CROW_ROUTE(app, "/api/audio/next")
    ([&](const crow::request& req) {
        if (!is_authorized(req)) {
            return crow::response(403, "Acceso denegado: Token inválido");
        }
        
        audio.nextSong();
        crow::response res(200, "Siguiente");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Siguiente");
    });

    CROW_ROUTE(app, "/api/audio/prev")
    ([&](const crow::request& req) {
        if (!is_authorized(req)) {
            return crow::response(403, "Acceso denegado: Token inválido");
        }
        
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
        if (!is_authorized(req)) {
            return crow::response(403, "Acceso denegado: Token inválido");
        }
        
        audio.forward5s();
        crow::response res(200, "+5s");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "+5s");
    });

    CROW_ROUTE(app, "/api/audio/back")
    ([&](const crow::request& req) {
        if (!is_authorized(req)) {
            return crow::response(403, "Acceso denegado: Token inválido");
        }
        
        audio.back5s();
        crow::response res(200, "-5s");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "-5s");
    });

    CROW_ROUTE(app, "/api/audio/volume/up")
    ([&](const crow::request& req) {
        if (!is_authorized(req)) {
            return crow::response(403, "Acceso denegado: Token inválido");
        }
        
        audio.upVolume();
        crow::response res(200, "Volumen +");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Volumen +");
    });

    CROW_ROUTE(app, "/api/audio/volume/down")
    ([&](const crow::request& req) {
        if (!is_authorized(req)) {
            return crow::response(403, "Acceso denegado: Token inválido");
        }
        
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
        if (!is_authorized(req)) {
            return crow::response(403, "Acceso denegado: Token inválido");
        }
        
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
        std::cout << "WS: Cliente conectado y autorizado." << std::endl;
    })
    .onclose([&](crow::websocket::connection& conn, const std::string& reason) {
        std::cout << "WS: Conexión cerrada: " << reason << std::endl;
    })
    .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
        if (active_tokens.find(data) != active_tokens.end()) {
            std::cout << "WS: Token validado correctamente." << std::endl;
            // Aquí puedes marcar la conexión como "autorizada" usando userdata
            return;
        }
        
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