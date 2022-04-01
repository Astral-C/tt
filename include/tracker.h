#include "mod.h"

typedef struct {
    uint16_t player_position;
    MODFile module;
} ModTracker;

// initialize a tracker for a .mod file with the given path
uint8_t tracker_open_mod(ModTracker* tracker, char* mod);
void tracker_close_mod(ModTracker* tracker);
