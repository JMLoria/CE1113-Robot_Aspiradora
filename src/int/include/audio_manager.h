#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <string>
#include <thread>
#include <atomic>

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    // Controles requeridos
    void play(const std::string& fileName);
    void pause();
    void stop();
    void setVolume(int volume);

    // Reproduccion de notificaciones cortas
    void playNotification(const std::string& eventType);

private:
    std::atomic<bool> isPlaying;
    std::atomic<int> currentVolume;
    std::thread audioThread;

    // Funcion interna que correra en el hilo separado
    void audioWorker(std::string path);
};

#endif // AUDIO_MANAGER_H