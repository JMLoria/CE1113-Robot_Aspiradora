#include "../include/audio_manager.h"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

AudioManager::AudioManager()
{
    loadPlaylist();

    // El control del reproductor corre en un hilo dedicado para no bloquear la UI.
    audio_thread = std::thread(&AudioManager::initProcess, this);
}

AudioManager::~AudioManager()
{
    running = false;
    stop();
    if (audio_thread.joinable())
    {
        audio_thread.join();
    }
    system("rm /tmp/mpg123_fifo 2>/dev/null");
}

void AudioManager::initProcess()
{
    // Se usa una FIFO local para comunicar comandos con el modo remoto de mpg123.
    system("mkfifo /tmp/mpg123_fifo 2>/dev/null");

    FILE *pipe = popen("mpg123 -a hw:0,0 -R --fifo /tmp/mpg123_fifo", "r"); 
    
    if (!pipe) {
        std::cerr << "[AUDIO] Error: No se pudo iniciar mpg123" << std::endl;
        return;
    }

    char buffer[256];
    while (running && fgets(buffer, sizeof(buffer), pipe))
    {
        std::string line(buffer);

        if (line.find("@F") == 0)
        {
            std::stringstream ss(line);
            std::string tag;
            double f_cur, f_left, s_cur, s_left;

            // mpg123 expone el tiempo actual y restante en segundos dentro del mensaje @F.
            ss >> tag >> f_cur >> f_left >> s_cur >> s_left;

            current_seconds = (int)s_cur;
            total_seconds = (int)(s_cur + s_left);
        }

        if (line.find("@P 0") != std::string::npos && is_playing_active)
        {
            // Cuando la pista termina y la reproducción sigue activa, avanza a la siguiente.
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            this->nextSong();
        }
    }
    pclose(pipe);
}

void AudioManager::loadPlaylist()
{
    playlist.clear();

    std::string music_dir = data_path + "musica/";

    try
    {
        if (fs::exists(music_dir) && fs::is_directory(music_dir))
        {
            for (const auto &entry : fs::directory_iterator(music_dir))
            {
                // Solo se consideran archivos MP3; el orden final se fija más abajo.
                if (entry.is_regular_file() && entry.path().extension() == ".mp3")
                {
                    // Se almacena el nombre base para exponerlo de forma estable en la UI.
                    playlist.push_back(entry.path().stem().string());
                }
            }
        }
        else
        {
            std::cerr << "[AUDIO] Error: La carpeta de musica no existe en " << music_dir << std::endl;
        }
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "[AUDIO] Error de sistema de archivos" << e.what() << std::endl;
    }

    std::sort(playlist.begin(), playlist.end());

    std::cout << "[AUDIO] Playlist cargada: " << playlist.size() << " canciones encontradas" << std::endl;
}

void AudioManager::sendCommand(const std::string &cmd)
{
    // El mutex evita intercalado de comandos cuando llegan varias peticiones simultáneas.
    std::lock_guard<std::mutex> lock(fifo_mutex);
    std::ofstream fifo("/tmp/mpg123_fifo");
    if (fifo.is_open())
    {
        fifo << cmd << std::endl;
        fifo.close();
    }
}

void AudioManager::play(int index)
{
    if (index == -1 && is_paused)
    {
        pause();
        return;
    }

    if (index != -1)
        current_track_index = index % playlist.size();

    is_playing_active = true;
    is_paused = false;

    // Se resuelve el índice contra la lista local antes de enviar LOAD a mpg123.
    std::string full_path = data_path + "musica/" + playlist[current_track_index] + ".mp3";
    sendCommand("LOAD " + full_path);
    sendCommand("VOLUME " + std::to_string(volume));
    std::cout << "[AUDIO] Reproduciendo: " << playlist[current_track_index] << "." << std::endl;
}

void AudioManager::pause()
{
    sendCommand("PAUSE");
    is_paused = !is_paused;
    if (is_paused)
    {
        std::cout << "[AUDIO] Pausa." << std::endl;
        return;
    }

    std::cout << "[AUDIO] Reproduciendo: " << playlist[current_track_index] << "." << std::endl;
}

void AudioManager::stop()
{
    // Detener limpia el estado de reproducción activa; no borra la selección actual.
    is_playing_active = false;
    is_paused = false;
    sendCommand("STOP");
    std::cout << "[AUDIO] Detenido." << std::endl;
}

void AudioManager::nextSong()
{
    is_paused = false;
    // El módulo de audio siempre cicla la lista; no existe un final "duro".
    current_track_index = (current_track_index + 1) % playlist.size();
    play();
}

void AudioManager::prevSong()
{
    is_paused = false;
    // Se usa suma modular para retroceder sin salir del rango de la playlist.
    current_track_index = (current_track_index - 1 + playlist.size()) % playlist.size();
    play();
}

void AudioManager::playSpecific(const std::string &songName)
{
    for (size_t i = 0; i < playlist.size(); i++)
    {
        if (playlist[i] == songName)
        {
            pause();
            std::cout << "[AUDIO] Selección manual: " << songName << " (Índice: " << i << ")" << std::endl;

            // Se actualiza el índice antes de delegar la reproducción a `play`.
            current_track_index = i;

            play(i);
            return;
        }
    }
}

void AudioManager::forward5s()
{
    // El salto temporal se delega directamente al reproductor remoto.
    sendCommand("JUMP +5s");
}

void AudioManager::back5s()
{
    // El salto temporal se delega directamente al reproductor remoto.
    sendCommand("JUMP -5s");
}

void AudioManager::upVolume()
{
    // El volumen se mantiene acotado para evitar valores inválidos en el backend.
    volume = std::min(volume + 5, 100);
    sendCommand("VOLUME " + std::to_string(volume));
}

void AudioManager::downVolume()
{
    // El volumen se mantiene acotado para evitar valores inválidos en el backend.
    volume = std::max(volume - 5, 0);
    sendCommand("VOLUME " + std::to_string(volume));
}

void AudioManager::notifications(const std::string &alert_name)
{
    // Las notificaciones se disparan fuera del flujo principal para no bloquear comandos de reproducción.
    std::thread([this, alert_name]()
                {
        std::string path = data_path + "sounds/" + alert_name + ".mp3";
        std::string cmd = "mpg123 -q " + path + " > /dev/null 2>&1";
        system(cmd.c_str()); })
        .detach();
}

int AudioManager::getCurrentTime()
{
    return current_seconds.load();
}

int AudioManager::getTotalTime()
{
    return total_seconds.load();
}

std::string AudioManager::getCurrentTrackName()
{
    return playlist[current_track_index];
}

int AudioManager::getVolume() const
{
    return volume.load();
}

std::vector<std::string> AudioManager::getPlaylist() const
{
    return playlist;
}
