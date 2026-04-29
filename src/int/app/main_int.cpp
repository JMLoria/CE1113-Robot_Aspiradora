#include "../include/crow_all.h"
#include "../include/audio_manager.h"
#include "../include/mapping.h"
#include "../include/user_manager.h"
#include "librobot.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <random>
#include <unordered_set> // Tokens de sesión válidos en memoria.


// Conjunto en memoria para validar accesos a los endpoints protegidos.
std::unordered_set<std::string> active_tokens;

// Genera un token efímero suficientemente impredecible para una sesión local.
std::string generate_token() {
    static const char alphabet[]= "abcdefghijklmnopqrstuvwxyz0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(alphabet) - 2);
    std::string token = "tk-";
    for (int i = 0; i < 16; ++i) token += alphabet[dis(gen)];
    return token;
}

// Verifica que la petición porte un token previamente emitido por el login.
bool is_authorized(const crow::request& req) {
    auto token = req.get_header_value("Authorization");
    if (token.empty()) return false;
    return active_tokens.find(token) != active_tokens.end();
}

int main() {
    if (robot_init() != ROBOT_OK) {
        std::cerr << "Error crítico: No se pudo inicializar el hardware." << std::endl;
        //return -1;
    }
    crow::SimpleApp app;

    UserManager auth("/home/data/r_users/users_robot.json");

    AudioManager audio; 
    MappingManager mapper(20, 20);

    // Estado local que el frontend consume para reflejar la orientación actual.
    int robot_angle = 0;
    // El mapa lógico arranca centrado para alinearse con la vista inicial.
    int cur_x = 10;
    int cur_y = 10;
    // Variables de estado que determinan la representación visual y la lógica de control.
    bool is_autonomous = false;
    bool obstacle_alert = false;
    bool system_on = true;

    // Señal sonora inicial para confirmar que el servicio quedó activo.
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

    CROW_ROUTE(app, "/app.js")  // Script principal de la interfaz web.
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

    CROW_ROUTE(app, "/auth.js") // Lógica del formulario y manejo de sesión en el navegador.
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

    CROW_ROUTE(app, "/style.css") // Estilos de presentación para la interfaz del panel.
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
    // Crea usuarios nuevos persistiendo solo el identificador hash y las credenciales derivadas.
    // MERGE CONFLICTS
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
    // Emite un token temporal en memoria si las credenciales coinciden.
    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST) // Ruta de login
    ([&auth, &audio](const crow::request& req) {
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

            audio.notifications("connect"); 

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
    // Invalida el token actual y deja la sesión del frontend sin privilegios.
    CROW_ROUTE(app, "/logout").methods(crow::HTTPMethod::POST)
    ([&](const crow::request& req) {
        auto token = req.get_header_value("Authorization");
        if (!token.empty()) {
            active_tokens.erase(token);
        }

        audio.notifications("disconnect"); 

        return crow::response(200, "{\"status\":\"success\"}");
    });

    CROW_ROUTE(app, "/api/<path>")
    .methods(crow::HTTPMethod::OPTIONS)
    ([](const crow::request& req, std::string path) {
        // Respuesta CORS preflight compartida por todos los endpoints del prefijo /api.
        crow::response res(204);
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Authorization, Content-Type");
        return res;
    });
  
    // --- ENPOINTS DE ESTADO ---
    // Cambia el modo operativo entre manual y autónomo desde la interfaz.
    // MERGE CONFLICTS --------------------------------------------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/mode/<string>")
    ([&is_autonomous](const crow::request& req, std::string mode) {
        crow::json::wvalue response_json;
        
        // 1. Verificación de Seguridad
        if (!is_authorized(req)) {
            response_json["error"] = "Acceso denegado: Token inválido";
            crow::response res(403, response_json);
            res.set_header("Access-Control-Allow-Origin", "*");
            return res;
        }

        // 2. Lógica de Control de Modo
        if (mode == "auto") {
            is_autonomous = true;
            robot_set_mode(MODE_AUTONOMOUS); // Llamada a la biblioteca física
            response_json["message"] = "Modo cambiado a auto";
        } else {
            is_autonomous = false;
            robot_set_mode(MODE_MANUAL);    // Llamada a la biblioteca física
            response_json["message"] = "Modo cambiado a manual";
        }

        // 3. Log en consola para depuración
        std::cout << "[MODO] Solicitud recibida: " << mode 
                  << " | Estado interno is_autonomous: " << is_autonomous << std::endl;

        // 4. Respuesta Exitosa con CORS
        crow::response res(200, response_json);
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        return res;
    });

// --- ENDPOINTS CONTROL REMOTO MANUAL ---  
    // MERGE CONFLICTS --------------------------------------------------------------------------------------------------------------
    CROW_ROUTE(app, "/api/move/<string>") 
    ([&](const crow::request& req, std::string dir) {
        crow::response res;
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Authorization, Content-Type");

        // 1. Verificación de Seguridad (Token)
        if (!is_authorized(req)) {
            res.code = 403;
            res.body = "{\"status\":\"error\", \"message\":\"No autorizado\"}";
            return res;
        }

        if (dir == "left") {
            // Rotación lógica
            robot_angle = (robot_angle - 90 + 360) % 360;
            // Rotación física
            robot_rotate(DIR_LEFT, 90.0f); 
            std::cout << "[MOTOR] Rotando Izquierda. Ángulo: " << robot_angle << std::endl;

        } else if (dir == "right") {
            // Rotación lógica
            robot_angle = (robot_angle + 90) % 360;
            // Rotación física
            robot_rotate(DIR_RIGHT, 90.0f); 
            std::cout << "[MOTOR] Rotando Derecha. Ángulo: " << robot_angle << std::endl;

        } else if (dir == "forward" || dir == "backward") {
            int step = (dir == "forward") ? 1 : -1;
            int next_x = cur_x;
            int next_y = cur_y;

            // Interpretación de dirección según orientación (Norte=0, Este=90, Sur=180, Oeste=270)
            if (robot_angle == 0)      next_y -= step;
            else if (robot_angle == 90)  next_x += step;
            else if (robot_angle == 180) next_y += step;
            else if (robot_angle == 270) next_x -= step;

            // 2. Verificación de Colisión y Movimiento
            if (mapper.isTraversable(next_x, next_y)) {
                cur_x = next_x;
                cur_y = next_y;
                
                // Movimiento físico (Solo si es seguro)
                if (dir == "forward") robot_move(DIR_FORWARD, 80);
                else robot_move(DIR_BACKWARD, 80);

                mapper.updateRobotPosition(cur_x, cur_y);
                std::cout << "[MOTOR] Moviendo a " << next_x << "," << next_y << std::endl;
            } else {
                // Bloqueo de seguridad: Detener motores si hay obstáculo en mapa
                robot_stop(); 
                audio.notifications("obstacle");
                std::cout << "[ALERTA] Movimiento bloqueado por mapa en " << next_x << "," << next_y << std::endl;
            }

        } else if (dir == "stop") {
            // Detención física
            robot_stop();
            
            // Lógica de reinicio (de la rama develop)
            mapper.resetMap();
            cur_x = 10;
            cur_y = 10;
            robot_angle = 0;
            
            audio.notifications("finish"); 
            std::cout << "[SISTEMA] Stop: Motores detenidos y mapa reiniciado." << std::endl;
        }

        res.code = 200;
        res.body = "{\"status\":\"success\"}";
        return res;
    });

    // --- ENDPOINTS MÚSICA ---
    // Reproduce la pista actual o una pista concreta mediante el manejador de audio.
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

    // Reproducción por índice, delegando la resolución real de la pista al gestor de audio.
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

    // Expone la lista de reproducción para poblar la selección en el navegador.
    CROW_ROUTE(app, "/api/audio/playlist")
    ([&audio](const crow::request& req) {
        crow::json::wvalue response_json; 
        
        if (!is_authorized(req)) {
            response_json["error"] = "Acceso denegado: Token inválido";
            crow::response res(403, response_json);
            res.set_header("Access-Control-Allow-Origin", "*");
            return res;
        }
        
        std::vector<std::string> songs = audio.getPlaylist(); 
        response_json["songs"] = songs; 

        crow::response res(200, response_json);
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // Reproduce una pista por nombre visible en la UI, no por ruta de archivo.
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

    // Pausa o reanuda el reproductor sin perder el punto de reproducción actual.
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

    // Detiene la reproducción y deja el sistema listo para iniciar otra pista.
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

    // Avanza a la siguiente pista de la lista cargada en memoria.
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

    // Retrocede a la pista anterior usando navegación circular sobre la playlist.
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
    // Expone controles de salto temporal y volumen sin acceder directamente al reproductor.
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

    // Expone controles de salto temporal y volumen sin acceder directamente al reproductor.
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

    // El frontend usa estos endpoints para ajustar el volumen en pasos discretos.
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

    // El frontend usa estos endpoints para ajustar el volumen en pasos discretos.
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
    // Dispara un audio corto de alerta sin interferir con la pista principal.
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
    // Genera obstáculos aleatorios para validar la representación del mapa desde la UI.
    CROW_ROUTE(app, "/api/test/obstacles")
    ([&](const crow::request& req) {
        std::cout << "[TEST] Generando obstáculos de prueba..." << std::endl;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> disX(0, 19);
        std::uniform_int_distribution<> disY(0, 19);

        // Se agregan obstáculos aleatorios para cubrir varios casos de visualización.
        for (int i = 0; i < 35; i++) {
            int obsX = disX(gen);
            int obsY = disY(gen);

            if (obsX != cur_x || obsY != cur_y) {
                mapper.addObstacle(obsX, obsY);  //EJEMPLO--------------------------------------------------------------------------------
            }
        }

        crow::response res(200, "Obstaculos generados.");
        // Esto permite que el archivo HTML local se comunique con el servidor
        res.set_header("Access-Control-Allow-Origin", "*"); 
        return res;
        // return crow::response(200, "Obstaculos generados.");
    });

    // --- WEBSOCKET ---
    // WebSocket de actualización: responde a `update` con el estado completo del sistema.
    // MERGE CONFLICTS --------------------------------------------------------------------------------------------------------------
   CROW_ROUTE(app, "/ws")
    .websocket()
    .onopen([&](crow::websocket::connection& conn) {
        std::cout << "WS: Cliente intentando conectar..." << std::endl;
    })
    .onclose([&](crow::websocket::connection& conn, const std::string& reason) {
        std::cout << "WS: Conexión cerrada: " << reason << std::endl;
    })
    .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
        
        // 1. Lógica de Validación de Token (Rama develop)
        // El primer mensaje que envía app.js es el token
        if (active_tokens.find(data) != active_tokens.end()) {
            std::cout << "WS: Token validado correctamente. Conexión autorizada." << std::endl;
            return;
        }

        // 2. Procesamiento de solicitud de actualización
        if (data == "update") {
            crow::json::wvalue response;
            
            // --- A. Lectura de Sensores Físicos (Rama fix/robot) ---
            robot_sensor_data_t s_data = {0};
            if (robot_sensor_read(&s_data) == ROBOT_OK) {
                // Si hay un objeto a menos de 20cm, activamos alerta y frenado
                if (s_data.front_cm < 20.0f && s_data.front_cm > 0.0f) {
                    obstacle_alert = true;
                    robot_stop(); // Seguridad: Detener motores físicamente

                    // Registrar obstáculo en el mapa según orientación actual
                    int obs_x = cur_x, obs_y = cur_y;
                    if (robot_angle == 0) obs_y--;
                    else if (robot_angle == 90) obs_x++;
                    else if (robot_angle == 180) obs_y++;
                    else if (robot_angle == 270) obs_x--;
                    
                    mapper.addObstacle(obs_x, obs_y);
                } else {
                    obstacle_alert = false;
                }
                
                // Añadir datos de sensores al JSON de respuesta
                response["sensors"]["front"] = s_data.front_cm;
                response["sensors"]["left"] = s_data.left_cm;
                response["sensor"]["unit"]  = "cm";
            }

            // --- B. Datos del Mapa (Rama develop/fix) ---
            auto map_data = crow::json::load(mapper.getMapAsJson());
            response["map"]["grid"] = map_data["grid"];
            response["map"]["robot"] = map_data["robot"];
            response["map"]["angle"] = robot_angle;

            // --- C. Datos de Audio (Rama develop) ---
            response["audio"]["track"] = audio.getCurrentTrackName();
            response["audio"]["current"] = audio.getCurrentTime();
            response["audio"]["total"] = audio.getTotalTime();
            response["audio"]["volume"] = audio.getVolume();

            // --- D. Estado Operativo y LEDs ---
            response["status"]["autonomous"] = is_autonomous;
            response["status"]["manual"] = !is_autonomous;
            response["status"]["obstacle"] = obstacle_alert;
            response["status"]["system"] = system_on;

            // Actualización física de LEDs en el robot
            robot_led_set(LED_POWER, system_on ? LED_ON : LED_OFF);
            robot_led_set(LED_AUTO, is_autonomous ? LED_ON : LED_OFF);
            robot_led_set(LED_MANUAL, !is_autonomous ? LED_ON : LED_OFF);
            robot_led_set(LED_OBSTACLE, obstacle_alert ? LED_ON : LED_OFF);

            // Enviar paquete consolidado al frontend
            conn.send_text(response.dump());
        }
    }); 

    std::cout << "\n==========================================" << std::endl;
    std::cout << "SERVIDOR INICADO" << std::endl;
    std::cout << "Abrir en navegador: http://localhost:8080" << std::endl;
    std::cout << "==========================================\n" << std::endl;

    app.loglevel(crow::LogLevel::Warning); // Reduce ruido de logs y deja visibles solo advertencias relevantes.
    app.port(8080).multithreaded().run();

    std::cout << "Apagando hardware..." << std::endl;
    robot_shutdown();
    return 0;
}
