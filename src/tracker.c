#include "tracker.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

uint8_t tracker_open_mod(ModTracker* tracker, char* mod){
	int i;
	FILE* mod_file;

	if(tracker == NULL) return 1; //oops

	tracker->bpm = 125; //defaults
	tracker->speed = 6;

	mod_file = fopen(mod, "r");
	fread(&tracker->module.module_name, sizeof(tracker->module.module_name), 1, mod_file);
	fread(&tracker->module.samples, sizeof(tracker->module.samples), 1, mod_file);

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
	uint32_t note;

	if(tracker->_tick_timer >= tracker->_updates_per_tick){
		if(tracker->_current_ticks % tracker->speed == 0){
			for (ch = 0; ch < 4; ch++){
				note = tracker->module.patterns[tracker->current_pattern].rows[tracker->current_row][ch];
				tracker->channels[ch].instrument = (note & 0x00F00000) >> 20; 
				printf("Pattern %d Row %d Channel %d playing Instrument %x\n", tracker->current_pattern, tracker->current_row, ch, note);
				tracker->channels[ch].volume = tracker->module.samples[tracker->channels[ch].instrument].volume;
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

// 37 50 ac 01
// 0011 0111 0101
void tracker_mod_set_sample_rate(ModTracker* tracker, uint32_t sampleRate){
	tracker->_updates_per_tick = sampleRate * 2.5 / tracker->bpm; 
	printf("Set updates per tick to %d\n", tracker->_updates_per_tick);
}

void tracker_mod_update(ModTracker* tracker, int16_t* buffer, uint32_t buf_size){
	int samples_per_tick, buff_ptr, poff;
	int16_t ch, samp_l, samp_r;
	Channel chan;

	while(buff_ptr < buf_size){
		tracker_mod_tick(tracker);

		for (ch = 0; ch < 4; ch++){
			//mix channel
			double freq = 8363 * pow((1152 - 808/ 192), 2);
			chan = tracker->channels[ch]; 
			if(tracker->module.sample_data[chan.instrument] == NULL) continue;
			samp_l += (tracker->module.sample_data[chan.instrument][(uint32_t)chan.sample_offset] - 128) * chan.volume;
			samp_r += (tracker->module.sample_data[chan.instrument][(uint32_t)chan.sample_offset] - 128) * chan.volume;
			
			if(chan.sample_offset += (freq / 44100) >= tracker->module.samples[chan.instrument].sample_length){
				chan.sample_offset = tracker->module.samples[chan.instrument].repeat_offset;
			}
		}
		
		buffer[buff_ptr] = samp_r;
		buffer[buff_ptr+1] = samp_l;
		buff_ptr += 2;
	} 
}