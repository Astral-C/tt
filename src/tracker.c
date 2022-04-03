#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "tracker.h"
#include "modeffect.h"

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

	memset(tracker, 0, sizeof(tracker));

	tracker->bpm = 125; //defaults
	tracker->speed = 6;

	mod_file = fopen(mod, "rb");
	fread(&tracker->module.module_name, sizeof(tracker->module.module_name), 1, mod_file);
	fread(&tracker->module.samples[0], sizeof(tracker->module.samples), 1, mod_file);
	for (i = 0; i < 32; i++){
		tracker->module.samples[i].sample_length = swap16(tracker->module.samples[i].sample_length);
		tracker->module.samples[i].repeat_length = swap16(tracker->module.samples[i].repeat_length);
		tracker->module.samples[i].repeat_offset = swap16(tracker->module.samples[i].repeat_offset);
	}
	

	fread(&tracker->module.song_length, sizeof(tracker->module.song_length), 1, mod_file); //length in patterns
	fread(&tracker->module.old_tracker_force, sizeof(tracker->module.old_tracker_force), 1, mod_file);
	fread(&tracker->module.song_positions, 128, 1, mod_file);
	fread(&tracker->module.signature, sizeof(tracker->module.signature), 1, mod_file);
	

	for(i = 0; i < 128; i++){
		if(tracker->module.song_positions[i] > tracker->module.pattern_count){
			tracker->module.pattern_count = tracker->module.song_positions[i];
		}
	}

	tracker->module.patterns = malloc(sizeof(MODPattern) * tracker->module.pattern_count);
	
	fread(&tracker->module.patterns[0], sizeof(MODPattern), tracker->module.pattern_count, mod_file);

	//read sample data
	for(i = 0; i < 32; i++){
		MODSampleDef* def = &tracker->module.samples[i];
		def->sample_length *= 2;
		def->repeat_offset *= 2;
		def->repeat_length *= 2;
		def->finetune <<= 4;

		tracker->module.sample_data[i] = malloc(def->sample_length);
		fread(tracker->module.sample_data[i], 1, def->sample_length, mod_file);
	}

	fclose(mod_file);

#ifdef debug_write
	tracker->dump = fopen("raw_pcm.pcm", "w");
#endif

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
#ifdef debug_write
	fclose(tracker->dump);
#endif
}

void tracker_mod_tick(ModTracker* tracker){
	int ch;
	uint8_t prev_instrument;
	uint32_t note;

	if(tracker->_tick_timer >= tracker->_updates_per_tick){
		printf("Ticking %d...\n", tracker->_current_ticks);
		if(tracker->_current_ticks == tracker->speed){
			for (ch = 0; ch < 4; ch++){
				note = swap32(tracker->module.patterns[tracker->module.song_positions[tracker->current_pattern]].rows[tracker->current_row][ch]);
				tracker->channels[ch].instrument = (note & 0xF0000000) >> 24 | (note & 0x0000F000) >> 12; 
				tracker->channels[ch].period = (note & 0x0FFF0000) >> 16; 
				tracker->channels[ch].porta_period = (note & 0x0FFF0000) >> 16; 
				tracker->channels[ch].effect = (note & 0x00000F00) >> 8;
				tracker->channels[ch].effect_args = (note & 0x000000FF);
				tracker->channels[ch].volume = tracker->module.samples[tracker->channels[ch].instrument].volume;
				tracker->channels[ch].sample_offset = 0;
			}
			

			tracker->current_row++;
			if(tracker->current_row == 64){
				tracker->current_row = 0;
				tracker->current_pattern++;
				if(tracker->current_pattern > tracker->module.song_length){
					tracker->current_pattern = 0;
				}
			}
			tracker->_current_ticks = 0;
		}

		//other per pick stuff like effects
		for (ch = 0; ch < 4; ch++){
			Channel* chan = &tracker->channels[ch];
			
			//Call effect from effect_list
			if (chan->effect > 0x00)
			{
				(*effect_list[chan->effect - 1])(tracker, chan);
			}
		}

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
	Channel* chan;
	int16_t mixed;
	buff_ptr = 0;

	printf("Buffer Size is %d\n", buf_size);

	while(buff_ptr < buf_size){
		samp_l = 0;
		samp_r = 0;
		tracker_mod_tick(tracker);

#ifdef debug_write
		mixed = 0;
#endif

		for (ch = 0; ch < 4; ch++){
			printf("begin mix %u\n", ch);
			chan = &tracker->channels[ch];
			if(chan->period == 0) continue;
			double freq = (((8363.0 * 428.0) / chan->period) / 44100.0);
			printf("===Mixing channel %u===\nPeriod %d\nfrequency %f\nSample Len %d\nSample Offest %d\nInstrument %d\n", ch, chan->period, freq, tracker->module.samples[chan->instrument].sample_length, chan->sample_offset, chan->instrument);
			if(tracker->module.sample_data[chan->instrument] == NULL) continue;
			
			int16_t sample = (int16_t)tracker->module.sample_data[chan->instrument][(uint32_t)chan->sample_offset];
			samp_l += sample * chan->volume;
			samp_r += sample * chan->volume;

			chan->sample_offset += freq;
			if(chan->sample_offset >= tracker->module.samples[chan->instrument].sample_length){
				if (tracker->module.samples[chan->instrument].repeat_length > 1)
				{
					//if loop length is more than 1, the sample loops (0 supposedly is unsupported but there's no docs)
					chan->sample_offset = tracker->module.samples[chan->instrument].repeat_offset + fmod(chan->sample_offset, tracker->module.samples[chan->instrument].repeat_length);
				}
				else
				{
					//no loop; stop note
					chan->period = 0;
					chan->sample_offset = 0;
				}
			}
		}
		
#ifdef debug_write
		fwrite(&samp_r, 2, 1, tracker->dump);
		fwrite(&samp_l, 2, 1, tracker->dump);
#endif		
		buffer[buff_ptr] = samp_r;
		buffer[buff_ptr+1] = samp_l;
		buff_ptr+=2;
	} 
}