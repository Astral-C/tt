#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "tracker.h"

//MOD Period Table

uint32_t swap32(uint32_t n) {
    return (((n>>24)&0xFF) | ((n<<8) & 0xFF0000) | ((n>>8)&0xFF00) | ((n<<24)&0xFF000000));
}

uint16_t swap16(uint16_t n) {
    return (((n<<8)&0xFF00) | ((n>>8)&0x00FF));
}

static int16_t period_table[60] = {
/*  C-0,  C#0,  D-0,  D#0,  E-0,  F-0,  F#0,  G-0,  G#0,  A-0,  A#0,  B-0, */
    1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016,  960,  906,
	
/*	C-1,  C#1,  D-1,  D#1,  E-1,  F-1,  F#1,  G-1,  G#1,  A-1,  A#1,  B-1, */
	856,  808,  762,  720,  678,  640,  604,  570,  538,  508,  480,  453,
	
/*	C-2,  C#2,  D-2,  D#2,  E-2,  F-2,  F#2,  G-2,  G#2,  A-2,  A#2,  B-2, */
	428,  404,  381,  360,  339,  320,  302,  285,  269,  254,  240,  226,

/*	C-3,  C#3,  D-3,  D#3,  E-3,  F-3,  F#3,  G-3,  G#3,  A-3,  A#3,  B-3, */
	214,  202,  190,  180,  170,  160,  151,  143,  135,  127,  120,  113,

/*	C-4,  C#4,  D-4,  D#4,  E-4,  F-4,  F#4,  G-4,  G#4,  A-4,  A#4,  B-4, */
	107,  101,   95,   90,   85,   80,   75,   71,   67,   63,   60,   56,
};

uint8_t tracker_open_mod(ModTracker* tracker, char* mod){
	int i;
	FILE* mod_file;

	if(tracker == NULL) return 1; //oops

	tracker->bpm = 125; //defaults
	tracker->speed = 6;

	mod_file = fopen(mod, "r");
	fread(&tracker->module.module_name, sizeof(tracker->module.module_name), 1, mod_file);
	fread(&tracker->module.samples, sizeof(tracker->module.samples), 1, mod_file);
	for (i = 0; i < 32; i++){
		tracker->module.samples[i].sample_length = swap16(tracker->module.samples[i].sample_length);
		tracker->module.samples[i].repeat_length = swap16(tracker->module.samples[i].repeat_length);
		tracker->module.samples[i].repeat_offset = swap16(tracker->module.samples[i].repeat_offset);
	}
	

	fread(&tracker->module.song_length, sizeof(tracker->module.song_length), 1, mod_file); //length in patterns
	fread(&tracker->module.old_tracker_force, sizeof(tracker->module.old_tracker_force), 1, mod_file);
	fread(&tracker->module.song_positions, sizeof(tracker->module.song_positions), 1, mod_file);
	fread(&tracker->module.signature, sizeof(tracker->module.signature), 1, mod_file);
	

	for(i = 0; i < 128; i++){
		if(tracker->module.song_positions[i] > tracker->module.pattern_count){
			tracker->module.pattern_count = tracker->module.song_positions[i];
		}
	}

	tracker->module.patterns = malloc(sizeof(MODPattern) * tracker->module.pattern_count);
	
	fread(&tracker->module.patterns[0], sizeof(MODPattern), tracker->module.pattern_count, mod_file);

	printf("Reading Samples at %x\n", ftell(mod_file));

	//read sample data
	for(i = 0; i < 32; i++){
		MODSampleDef* def = &tracker->module.samples[i];
		def->sample_length *= 2;
		def->repeat_offset *= 2;
		def->repeat_length *= 2;
		def->finetune <<= 4;
		//only read if the sample len is > 0
		if(def->sample_length == 0) continue;

		tracker->module.sample_data[i] = malloc(def->sample_length);
		fread(tracker->module.sample_data[i], def->sample_length, 1, mod_file);
	}

	fclose(mod_file);

	memset(tracker->channels, 0, sizeof(tracker->channels));
}


void tracker_close_mod(ModTracker* tracker){
	int i;

	if(tracker == NULL) return;
	if(tracker->module.patterns != NULL) free(tracker->module.patterns);

	for(i = 0; i < 32; i++){
		if(tracker->module.sample_data[i] != NULL && tracker->module.samples[i].sample_length > 0){
			free(tracker->module.sample_data[i]);
		}
	}
	
}

void tracker_mod_tick(ModTracker* tracker){
	int ch;
	uint8_t prev_instrument;
	uint32_t note;

	if(tracker->_tick_timer >= tracker->_updates_per_tick){
		if(tracker->_current_ticks % tracker->speed == 0){
			for (ch = 0; ch < 4; ch++){
				note = swap32(tracker->module.patterns[tracker->current_pattern].rows[tracker->current_row][ch]);
				tracker->channels[ch].instrument = (note & 0x000FF000) >> 12; 
				tracker->channels[ch].period = (note & 0xFFF00000) >> 20; 
				tracker->channels[ch].volume = tracker->module.samples[tracker->channels[ch].instrument].volume;
				tracker->channels[ch].sample_offset = 0;
			}
			

			tracker->current_row++;
			if(tracker->current_row == 64){
				tracker->current_row = 0;
				tracker->current_pattern++;
				if(tracker->current_pattern > tracker->module.pattern_count){
					tracker->current_pattern = 0;
				}
			}
		}

		//other per pick stuff like effects

		tracker->_current_ticks++;
		tracker->_tick_timer = 0;
	}
	tracker->_tick_timer++;
}


void tracker_mod_set_sample_rate(ModTracker* tracker, uint32_t sampleRate){
	tracker->_updates_per_tick = sampleRate * 2.5 / tracker->bpm; 
	printf("Set updates per tick to %d\n", tracker->_updates_per_tick);
}

void tracker_mod_update(ModTracker* tracker, int16_t* buffer, uint32_t buf_size){
	int samples_per_tick, buff_ptr;
	int16_t ch, samp_l, samp_r;
	Channel chan;
	buff_ptr = 0;

	while(buff_ptr < buf_size){
		samp_l = 0;
		samp_r = 0;
		tracker_mod_tick(tracker);

		for (ch = 0; ch < 4; ch++){
			//mix channel

			chan = tracker->channels[ch];
			if(chan.period == 0) continue;
			if(chan.sample_offset >= tracker->module.samples[chan.instrument].sample_length) continue;
			printf("Channel Period %d\n", chan.period);
			double freq = ((8363.0 * 428.0) / chan.period) / 44100.0;
			if(tracker->module.sample_data[chan.instrument] == NULL) continue;
			
			printf("Mixing Channel %d\n", ch);

			samp_l += (tracker->module.sample_data[chan.instrument][chan.sample_offset]) * chan.volume * 128;
			samp_r += (tracker->module.sample_data[chan.instrument][chan.sample_offset]) * chan.volume * 128;
			
			if(chan.sample_offset += freq >= tracker->module.samples[chan.instrument].sample_length){
				chan.sample_offset = tracker->module.samples[chan.instrument].repeat_offset;
			}
			printf("Finished Mixing Channel %d\n", ch);
		}
		
		buffer[buff_ptr] = samp_r;
		buffer[buff_ptr+1] = samp_l;
		buff_ptr += 2;
	} 
}