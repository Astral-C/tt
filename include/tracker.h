#ifndef __TRACKER_H__
#define __TRACKER_H__

#include "mod.h"

typedef struct {
    uint8_t instrument;
    uint8_t base_note;
    uint8_t volume;
    double sample_offset;
    uint32_t period;
} Channel;

typedef struct {
    uint32_t _tick_timer;
    uint32_t _current_ticks;
    uint32_t _updates_per_tick;

    uint16_t bpm;
    uint16_t speed;
    uint16_t current_row;
    uint16_t current_pattern;
    Channel channels[8]; //playback channels
    MODFile module;
} ModTracker;

// initialize a tracker for a .mod file with the given path
uint8_t tracker_open_mod(ModTracker* tracker, char* mod);
void tracker_close_mod(ModTracker* tracker);
void tracker_mod_update(ModTracker* tracker, int16_t* buffer, uint32_t buf_size);
void tracker_mod_set_sample_rate(ModTracker* tracker, uint32_t sampleRate);

#endif