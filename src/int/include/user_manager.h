#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <string>
#include <vector>
#include <mutex>
#include "sha256.h"

/**
 * @brief Representa un usuario persistido con credenciales anonimizadas y estado de bloqueo.
 */
struct User
{
    std::string user_id_hash;  // Hash del username sanitizado
    std::string password_hash; // Has de (password + salt)
    std::string salt;          // Sal aleatoria para la contraseña
    int failed_attempts;       // Contador de intentos [0,3]
    long long lockout_until;   // Timestamp Unix de desbloqueo (0 si no esta bloqueado)
};

/**
 * @brief Gestiona registro, autenticación, persistencia y bloqueo temporal de usuarios.
 */
class UserManager
{
public:
    UserManager(const std::string &db_path);
    ~UserManager();

    // Operaciones principales de ciclo de vida de usuario.
    bool registerUser(const std::string &username, const std::string &password);
    bool authenticate(const std::string &username, const std::string &password);

    // Consultas de estado expuestas al flujo de autenticación.
    bool userExists(const std::string &username);
    bool isLocked(const std::string &username);
    long long getRemainingLockTime(const std::string &username);

    // Validaciones y utilidades de recuperación.
    bool isValidUsername(const std::string &username);
    void resetAttempts(const std::string &username);

private:
    std::string db_file_path;
    std::vector<User> users;
    std::mutex user_mutex;

    // Utilidades internas para persistencia y búsqueda.
    void loadUsers();
    void saveUsers();
    std::string generateSalt(size_t length = 16);
    User *findUserByHash(const std::string &username);

    // El identificador almacenado es un hash para evitar persistir el nombre real.
    std::string getIdentifierHash(const std::string &username)
    {
        return SHA256::hash(username);
    }
};

#endif