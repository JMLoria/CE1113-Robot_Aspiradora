#include "librobot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

#define SOUNDS_PATH "/usr/share/robot/sounds/"
#define FIFO_PATH "/tmp/mpg123_fifo"

static int audio_daemon_running = 0;

// Inicializa el reproductor en segundo plano
static void init_audio_daemon() {
    if (!audio_daemon_running) {
        unlink(FIFO_PATH);            // Limpiar tuberías previas
        mkfifo(FIFO_PATH, 0666);      // Crear nueva tubería FIFO
        
        // Iniciar mpg123 en modo remoto, leyendo comandos desde el FIFO
        system("mpg123 -R --fifo " FIFO_PATH " > /dev/null 2>&1 &");
        audio_daemon_running = 1;
    }
}

// Función auxiliar para enviar comandos al proceso mpg123
static robot_status_t send_audio_command(const char *cmd) {
    init_audio_daemon();
    
    // O_NONBLOCK evita que el hilo se quede pegado si mpg123 falla
    int fd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
        fprintf(stderr, "[Audio HW] Error al abrir el FIFO de audio.\n");
        return ROBOT_ERR_HW;
    }
    
    dprintf(fd, "%s\n", cmd);
    close(fd);
    return ROBOT_OK;
}

robot_status_t robot_audio_play(const char *filepath) {
    char cmd[512];
    // Si la ruta no empieza con '/', asumimos que es relativa a SOUNDS_PATH
    if (filepath[0] == '/') {
        snprintf(cmd, sizeof(cmd), "LOAD %s", filepath);
    } else {
        snprintf(cmd, sizeof(cmd), "LOAD %s%s", SOUNDS_PATH, filepath);
    }
    return send_audio_command(cmd);
}

robot_status_t robot_audio_pause(void) {
    return send_audio_command("PAUSE");
}

robot_status_t robot_audio_stop(void) {
    return send_audio_command("STOP");
}

robot_status_t robot_audio_set_volume(uint8_t volume) {
    char cmd[64];
    // Evitar que el volumen exceda el 100% para no saturar el DAC de la Raspberry Pi
    if (volume > 100) volume = 100;
    snprintf(cmd, sizeof(cmd), "VOLUME %d", volume);
    return send_audio_command(cmd);
}

robot_status_t robot_audio_get_list(char ***files, int *count) {
    DIR *d;
    struct dirent *dir;
    int index = 0;
    
    d = opendir(SOUNDS_PATH);
    if (d) {
        // Contar archivos primero para reservar memoria
        int total_files = 0;
        while ((dir = readdir(d)) != NULL) {
            if (strstr(dir->d_name, ".mp3")) total_files++;
        }
        rewinddir(d);
        
        *files = malloc(total_files * sizeof(char*));
        if (!*files) return ROBOT_ERR_HW;
        
        while ((dir = readdir(d)) != NULL) {
            if (strstr(dir->d_name, ".mp3")) {
                (*files)[index] = strdup(dir->d_name);
                index++;
            }
        }
        closedir(d);
        *count = index;
        return ROBOT_OK;
    }
    
    *count = 0;
    return ROBOT_ERR_HW;
}