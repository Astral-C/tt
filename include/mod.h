#ifndef __MOD_H__
#define __MOD_H__

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

#include <stdint.h>

PACK(typedef struct) {
    char name[22];
    uint16_t sample_length; //in u16s
    int8_t finetune; // First nybble should be blank, next is finetune
    int8_t volume; //max is 64, 0x00 - 0x40
    uint16_t repeat_offset; //in u16s
    uint16_t repeat_length; //in u16s
} MODSampleDef;

PACK(typedef struct) { //we don't want any padding, we are reading this directly from the file
    uint32_t rows[64][4]; //only supporting 4 channels for now
} MODPattern;

typedef struct {
    char module_name[20];
    MODSampleDef samples[31];
    uint8_t song_length;
    uint8_t old_tracker_force; //always 127.
    uint8_t song_positions[128]; //value from 0-63, maps pattern-position
    // not sure what to call this? Some use 'M.K.' to signify 31 samples instead of 15, stattrekker puts FLT4 or FLT8 which mean nothing
    // some put nothing there to make it harder to tell how this mod works
    // see https://github.com/vlohacks/misc/blob/master/modplay/docs/FMODDOC.TXT#L2668
    // also 4chn and 8chn to determine channel count
    char signature[4];


    // not part of the header
    uint8_t pattern_count;

    MODPattern* patterns;
    int8_t* sample_data[31]; 
} MODFile;

#endif