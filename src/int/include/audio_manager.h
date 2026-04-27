#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

class AudioManager
{
public:
    AudioManager();
    ~AudioManager();

    // Control del flujo de reproducción. `index = -1` reutiliza el estado actual.
    void play(int index = -1);
    void pause();
    void stop();
    void nextSong();
    void prevSong();

    // Selección explícita de una pista por nombre visible en la interfaz.
    void playSpecific(const std::string &songName);

    // Desplazamiento relativo dentro de la pista en reproducción.
    void forward5s();
    void back5s();

    // Ajustes del volumen y reproducción de señales sonoras del sistema.
    void upVolume();
    void downVolume();
    void notifications(const std::string &alert_name);

    // Lectura del estado expuesto a la UI y al WebSocket.
    int getCurrentTime();
    int getTotalTime();
    std::string getCurrentTrackName();
    int getVolume() const;
    std::vector<std::string> getPlaylist() const;

private:
    std::string data_path = "../../../data/";
    std::vector<std::string> playlist;
    int current_track_index = 0;
    std::atomic<int> volume{50};

    std::atomic<int> current_seconds{0};
    std::atomic<int> total_seconds{0};

    std::atomic<bool> is_playing_active{false};
    bool is_paused = false;

    std::thread audio_thread;
    std::mutex fifo_mutex;
    std::atomic<bool> running{true};

    // Escribe comandos en la FIFO controlada por mpg123.
    void sendCommand(const std::string &cmd);
    // Reconstruye la lista de reproducción a partir del directorio de música.
    void loadPlaylist();
    // Bucle persistente que mantiene el proceso de audio y actualiza métricas.
    void initProcess();
};

#endif // AUDIO_MANAGER_H