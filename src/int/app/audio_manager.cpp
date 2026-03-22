#include "../include/audio_manager.h"
#include <iostream>
#include <cstdlib>                      // Para system()

// Definicion de rutas base de las canciones y sonidos
const std::string SOUNDS_PATH = "../../data/sounds/";
const std::string MUSICS_PATH = "../../dta/musics/";


AudioManager::AudioManager() : isPlaying(false), currentVolume(50) {}

AudioManager::~AudioManager() {
    stop();
}

void AudioManager::play(const std::string& fileName) {
    stop(); // Detener cualquier audio previo
    isPlaying = true;

    std::string fullPath = MUSICS_PATH + fileName;

    audioThread = std::thread(&AudioManager::audioWorker, this, fullPath);
    audioThread.detach();
}

void AudioManager::audioWorker(std::string path) {
    std::cout << "Reproduciendo: " << path << " al " << currentVolume << "% de volumen." << std::endl;

    // Comando para ejecutar en Linux (Yocot debe tener mpg123 instalado)
    std::string command = "mpg123 -q " + path;

    while(isPlaying) {
        std::system(command.c_str());
        isPlaying = false; // Termina tras una ejecucion (o puedes loopear)
    }
}

void AudioManager::setVolume(int volume) {
    currentVolume = volume;
    // Comando para ajustar volumen maestro en la APi
    std::string volCmd = "amixer sset 'Headphone' " + std::to_string(volume) + "%";
    std::system(volCmd.c_str());
}

void AudioManager::playNotification(const std::string& eventType) {
    std::string soundFile; 

    // Mapeo de eventos
    if(eventType == "INICIO") soundFile = "start.mp3";
    else if(eventType == "OBSTACULO") soundFile = "obstacle.mp3";
    else if(eventType == "MANUAL") soundFile = "mode.mp3";

    if (!soundFile.empty()) {
        std::string fullPath = SOUNDS_PATH + soundFile;
        // Reproduccion simple para notificaciones
        std::system(("mpg123 -q " + fullPath + " &").c_str());
    }
}

void AudioManager::stop() {
    isPlaying = false;
    std::system("killall mpg123 2>/dev/null");
    // En Linux, podrias necesitar matar el proceso: std::system("killall mpg123");
    if (audioThread.joinable()) {
        audioThread.join();
    }
}