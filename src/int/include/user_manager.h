#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <string>
#include <vector>
#include <mutex>
#include "sha256.h"

/**
 * @brief Estructura que representa a un usuario anonimizado en el sistema
 */
struct User {
    std::string user_id_hash;   // Hash del username sanitizado
    std::string password_hash;  // Has de (password + salt)
    std::string salt;           // Sal aleatoria para la contraseña
    int failed_attempts;        // Contador de intentos [0,3]
    long long lockout_until;    // Timestamp Unix de desbloqueo (0 si no esta bloqueado)
};

/**
 * @brief Clase encargada de la gestion de usuarios, persistencia y seguridad
 */
class UserManager {
public:
    UserManager(const std::string& db_path);
    ~UserManager();

    // Acciones principales
    bool registerUser(const std::string& username, const std::string& password);
    bool authenticate(const std::string& username, const std::string& password);

    // Verificaciones de estado
    bool userExists(const std::string& username);
    bool isLocked(const std::string& username);
    long long getRemainingLockTime(const std::string& username);

    // Seguridad y Limpieza
    std::string sanitizeInput(std::string input);
    void resetAttempts(const std::string& username);

private:
    std::string db_file_path;
    std::vector<User> users;
    std::mutex user_mutex;

    // Utilidades internas
    void loadUsers();
    void saveUsers();
    std::string generateSalt(size_t length = 16);
    User* findUserByHash(const std::string& username);

    // Helper para anonimizacion
    std::string getIdentifierHash(const std::string& username) {
        return SHA256::hash(sanitizeInput(username));
    }
};

#endif