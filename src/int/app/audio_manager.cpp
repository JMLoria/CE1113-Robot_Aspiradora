#include "../include/audio_manager.h"
#include <iostream>
#include <cstdlib>                     
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>


AudioManager::AudioManager() {
    loadPlaylist();

    // Creamos el hilo de ejecucion para el proceso de audio
    audio_thread = std::thread(&AudioManager::initProcess, this);
}

AudioManager::~AudioManager() {
    running = false;
    stop();
    if (audio_thread.joinable()) {
        audio_thread.join();
    }
    system("rm /tmp/mpg123_fifo 2>/dev/null");
}

void AudioManager::initProcess() {
    // Preparar la cola
    system("mkfifo /tmp/mpg123_fifo 2>/dev/null");

    FILE* pipe = popen("mpg123 -R --fifo /tmp/mpg123_fifo", "r");
    if (!pipe) return;

    char buffer[256];
    while(running && fgets(buffer, sizeof(buffer), pipe)) {
        std::string line(buffer);

        if (line.find("@F") == 0) {
            std::stringstream ss(line);
            std::string tag;
            double f_cur, f_left, s_cur, s_left;
            
            // El formato de mpg123 es: @F actual restante seg_actual seg_restante
            ss >> tag >> f_cur >> f_left >> s_cur >> s_left;
            
            current_seconds = (int)s_cur;
            total_seconds = (int)(s_cur + s_left);
        }

        if (line.find("@P 0") != std::string::npos && is_playing_active) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            this->nextSong();
        }
    }
    pclose(pipe);

}

void AudioManager::loadPlaylist() {
    playlist = {
        "Blacklight [Ado] - 2022",
        "Odo [Ado] - 2021",
        "Show [Ado] - 2023",
        "Usseewa [Ado] - 2020",
        "Vivarium [Ado] - 2026"
    };
}

void AudioManager::sendCommand(const std::string& cmd) {
    // El mutex evita que multiples peticiones web choquen al escribir en la FIFO
    std::lock_guard<std::mutex> lock(fifo_mutex);
    std::ofstream fifo("/tmp/mpg123_fifo");
    if(fifo.is_open()) {
        fifo << cmd << std::endl;
        fifo.close();
    }
}

void AudioManager::play(int index) {
    if (index == -1 && is_paused) {
        pause();
         return;
    } 

    if (index != -1) current_track_index = index % playlist.size();

    is_playing_active = true;
    is_paused = false;

    std::string full_path = data_path + "musics/" + playlist[current_track_index] + ".mp3";
    sendCommand("LOAD " + full_path);
    std::cout << "[AUDIO] Reproduciendo: " << playlist[current_track_index] << "." << std::endl;
}

void AudioManager::pause() {
    sendCommand("PAUSE");
    is_paused =! is_paused;
    if (is_paused) {
        std::cout << "[AUDIO] Pausa." << std::endl;
        return;
    }

    std::cout << "[AUDIO] Reproduciendo: " << playlist[current_track_index] << "." << std::endl;
}

void AudioManager::stop() {
    is_playing_active = false;
    is_paused = false;
    sendCommand("STOP");
    std::cout << "[AUDIO] Detenido." << std::endl;
}

void AudioManager::nextSong() {
    is_paused = false;
    current_track_index = (current_track_index + 1) % playlist.size();
    play(); 
}

void AudioManager::prevSong() {
    is_paused = false; 
    current_track_index = (current_track_index - 1 + playlist.size()) % playlist.size();
    play(); 
}

void AudioManager::forward5s() {
    sendCommand("JUMP +5s");
}

void AudioManager::back5s() {
    sendCommand("JUMP -5s");
}

void AudioManager::upVolume() {
    volume = std::min(volume + 5, 100);
    sendCommand("VOLUME " + std::to_string(volume));
}

void AudioManager::downVolume() {
    volume = std::max(volume - 5, 0);
    sendCommand("VOLUME " + std::to_string(volume));
}

void AudioManager::notifications(const std::string& alert_name) {
    // Las notificaciones corren en un hilo efimero para no interrumpir el flujo de control de la musica principal
    std::thread([this, alert_name]() {
        std::string path = data_path + "sounds/" + alert_name + ".mp3";
        std::string cmd = "mpg123 -q " + path + " > /dev/null 2>&1";
        system(cmd.c_str());
    }).detach();
}

int AudioManager::getCurrentTime() {
    return current_seconds.load();
}

int AudioManager::getTotalTime() {
    return total_seconds.load();
}

std::string AudioManager::getCurrentTrackName() {
    return playlist[current_track_index];
}