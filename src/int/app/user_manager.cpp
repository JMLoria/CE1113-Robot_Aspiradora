#include "../include/user_manager.h"
#include "../include/crow_all.h"
#include <fstream>
#include <iostream>
#include <random>
#include <ctime>
#include <regex>
#include <algorithm>

UserManager::UserManager(const std::string& db_path) : db_file_path(db_path) {
    loadUsers();
}

UserManager::~UserManager() {
    saveUsers();
}

bool UserManager::isValidUsername(const std::string& username) {
    if (username.empty()) return false;
    // Regex que busca cualquier cosa que NO sea alfanumérica
    std::regex re("^[a-zA-Z0-9]+$");
    return std::regex_match(username, re);
}

std::string UserManager::generateSalt(size_t length) {
    const std::string chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, chars.size() - 1);

    std::string salt;
    for (size_t i = 0; i < length; ++i) {
        salt += chars[distribution(generator)];
    }
    return salt;
}

void UserManager::loadUsers() {
    std::lock_guard<std::mutex> lock(user_mutex);
    std::ifstream file(db_file_path);
    if (!file.is_open()) return;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto json_data = crow::json::load(content);

    if (json_data && json_data.has("users")) {
        users.clear();
        for (auto& item : json_data["users"]) {
            User u;
            u.user_id_hash = item["user_id_hash"].s();
            u.password_hash = item["password_hash"].s();
            u.salt = item["salt"].s();
            u.failed_attempts = item["failed_attempts"].i();
            u.lockout_until = item["lockout_until"].i();
            users.push_back(u);
        }
    }
    file.close();
}

void UserManager::saveUsers() {
    std::lock_guard<std::mutex> lock(user_mutex);
    crow::json::wvalue json_output;
    std::vector<crow::json::wvalue> user_list;

    for (const auto& u : users) {
        crow::json::wvalue user_obj;
        user_obj["user_id_hash"] = u.user_id_hash;
        user_obj["password_hash"] = u.password_hash;
        user_obj["salt"] = u.salt;
        user_obj["failed_attempts"] = u.failed_attempts;
        user_obj["lockout_until"] = u.lockout_until;
        user_list.push_back(std::move(user_obj));
    }

    json_output["users"] = std::move(user_list);
    std::ofstream file(db_file_path);
    file << json_output.dump();
    file.close();
}

bool UserManager::registerUser(const std::string& username, const std::string& password) {
    if (!isValidUsername(username)) {
        std::cout << "[AUTH] Registro rechazado: Caracteres inválidos en '" << username << "'" << std::endl;
        return false; 
    }

    std::string id_hash = getIdentifierHash(username);
    if (findUserByHash(id_hash)) return false;

    User newUser;
    newUser.user_id_hash = id_hash;
    newUser.salt = generateSalt();
    newUser.password_hash = SHA256::hash(password + newUser.salt);
    newUser.failed_attempts = 0;
    newUser.lockout_until = 0;

    {
        std::lock_guard<std::mutex> lock(user_mutex);
        users.push_back(newUser);
    }
    saveUsers();
    return true;
}

bool UserManager::authenticate(const std::string& username, const std::string& password) {
    std::string id_hash = getIdentifierHash(username);
    User* u = findUserByHash(id_hash);

    if (!u) return false;

    // Verificar si está bloqueado
    long long now = static_cast<long long>(std::time(nullptr));
    if (u->lockout_until > now) return false;

    // Validar contraseña
    std::string login_hash = SHA256::hash(password + u->salt);
    if (u->password_hash == login_hash) {
        u->failed_attempts = 0;
        u->lockout_until = 0;
        saveUsers();
        return true;
    } else {
        u->failed_attempts++;
        if (u->failed_attempts >= 3) {
            u->lockout_until = now + 300; // Bloqueo de 5 min
        }
        saveUsers();
        return false;
    }
}

User* UserManager::findUserByHash(const std::string& user_hash) {
    for (auto& u : users) {
        if (u.user_id_hash == user_hash) return &u;
    }
    return nullptr;
}

bool UserManager::isLocked(const std::string& username) {
    User* u = findUserByHash(getIdentifierHash(username));
    if (!u) return false;
    return static_cast<long long>(std::time(nullptr)) < u->lockout_until;
}

bool UserManager::userExists(const std::string& username) {
    std::string id_hash = getIdentifierHash(username);
    return findUserByHash(id_hash) != nullptr;
}

long long UserManager::getRemainingLockTime(const std::string& username) {
    std::string id_hash = getIdentifierHash(username);
    User* u = findUserByHash(id_hash);
    
    if (!u || u->lockout_until == 0) return 0;

    long long now = static_cast<long long>(std::time(nullptr));
    long long remaining = u->lockout_until - now;
    
    return (remaining > 0) ? remaining : 0;
}

void UserManager::resetAttempts(const std::string& username) {
    std::string id_hash = getIdentifierHash(username);
    User* u = findUserByHash(id_hash);
    
    if (u) {
        std::lock_guard<std::mutex> lock(user_mutex);
        u->failed_attempts = 0;
        u->lockout_until = 0;
        saveUsers(); // Guardamos el cambio en el JSON
    }
}