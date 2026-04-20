#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    // Controles de flujo
    void play(int index = -1);
    void pause();
    void stop();
    void nextSong();
    void prevSong();

    void playSpecific(const std::string& songName);

    // Controles de tiempo
    void forward5s();
    void back5s();

    // Controles de audio
    void upVolume();
    void downVolume();
    void notifications(const std::string& alert_name);

    // Funciones de datos
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


    void sendCommand(const std::string& cmd);
    void loadPlaylist();
    void initProcess(); // Funcion que correra en el hilo
};

#endif // AUDIO_MANAGER_H