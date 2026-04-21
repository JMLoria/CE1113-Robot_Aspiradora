#define SOUNDS_PATH "/usr/share/robot/sounds/"
#include "librobot.h"
#include <stddef.h>

robot_status_t robot_audio_play(const char *filepath) { return ROBOT_OK; }
robot_status_t robot_audio_pause(void) { return ROBOT_OK; }
robot_status_t robot_audio_stop(void) { return ROBOT_OK; }
robot_status_t robot_audio_set_volume(uint8_t volume) { return ROBOT_OK; }
robot_status_t robot_audio_get_list(char ***files, int *count) { 
    if(count) *count = 0; 
    return ROBOT_OK; 
}