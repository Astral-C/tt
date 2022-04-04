#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "tracker.h"
#include "modeffect.h"

uint32_t swap32(uint32_t n) {
	return (((n>>24)&0xFF) | ((n<<8) & 0xFF0000) | ((n>>8)&0xFF00) | ((n<<24)&0xFF000000));
}

uint16_t swap16(uint16_t n) {
	return (((n<<8)&0xFF00) | ((n>>8)&0x00FF));
}

uint8_t tracker_open_mod(ModTracker* tracker, char* mod){
	int i;
	FILE* mod_file;

	if(tracker == NULL) return 1; //oops

	memset(tracker, 0, sizeof(ModTracker));

	tracker->bpm = 125; //defaults
	tracker->speed = 6;

	mod_file = fopen(mod, "rb");
	fread(&tracker->module.module_name, sizeof(tracker->module.module_name), 1, mod_file);
	fread(&tracker->module.samples[0], sizeof(MODSampleDef), 31, mod_file);

	for (i = 0; i < 31; i++){
		tracker->module.samples[i].sample_length = swap16(tracker->module.samples[i].sample_length);
		tracker->module.samples[i].repeat_length = swap16(tracker->module.samples[i].repeat_length);
		tracker->module.samples[i].repeat_offset = swap16(tracker->module.samples[i].repeat_offset);
	}

	fread(&tracker->module.song_length, sizeof(tracker->module.song_length), 1, mod_file); //length in patterns
	fread(&tracker->module.old_tracker_force, sizeof(tracker->module.old_tracker_force), 1, mod_file);
	fread(&tracker->module.song_positions, 1, 128, mod_file);
	fread(&tracker->module.signature, sizeof(tracker->module.signature), 1, mod_file);
	
	for(i = 0; i < 128; i++){
		if(tracker->module.song_positions[i] > tracker->module.pattern_count){
			tracker->module.pattern_count = tracker->module.song_positions[i];
		}
	}

	//The playlist only contains the highest index of the pattern; we'll add one to get the count
	tracker->module.pattern_count++;

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
	uint8_t instrument, effect, effect_args;
	uint32_t note, period;

	if(tracker->_tick_timer >= tracker->_updates_per_tick){
		if(tracker->_current_ticks >= tracker->speed){
			//printf("row update: tick %u\n", tracker->_current_ticks);
			
			for (ch = 0; ch < 4; ch++){
				note = swap32(tracker->module.patterns[tracker->module.song_positions[tracker->current_pattern]].rows[tracker->current_row][ch]);
				instrument = (note & 0xF0000000) >> 24 | (note & 0x0000F000) >> 12;	
				period = (note & 0x0FFF0000) >> 16;
				prev_instrument = tracker->channels[ch].instrument; 
				effect = (note & 0x00000F00) >> 8;
				effect_args = (note & 0x000000FF);
				tracker->channels[ch].effect = effect;

				if (effect_args != 0)
					tracker->channels[ch].effect_args = effect_args;

				if(instrument > 0 && instrument < 32){
					tracker->channels[ch].instrument = instrument - 1;
					tracker->channels[ch].pan = 0x80; //reset pan to middle
					tracker->channels[ch].volume = tracker->module.samples[tracker->channels[ch].instrument].volume;

					if(instrument != prev_instrument)
						tracker->channels[ch].sample_offset = 0;

					//printf("Channel %u set instrument to %u (row %u, pattern %u)\n", ch, instrument, tracker->current_row, tracker->current_pattern);
				}

				if(period > 0){
					//Effects 3xx and 5xx (toneporta and volslide + toneporta) override period assignment
					if (effect != 0x03 && effect != 0x05)
						tracker->channels[ch].period = period; 
					
					tracker->channels[ch].porta_period = period;

					//printf("Channel %u set period to %u (row %u, pattern %u)\n", ch, period, tracker->current_row, tracker->current_pattern);
				}

				/*if (effect != 0x00 && tracker->channels[ch].effect_args != 0x00)
				{
					printf("Channel %u Calling effect %x with params %x\n", ch, effect, tracker->channels[ch].effect_args);
				}*/
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
			(*effect_list[chan->effect])(tracker, chan);
		}

		tracker->_current_ticks++;
		tracker->_tick_timer = 0;
	}
	tracker->_tick_timer++;
}


void tracker_mod_set_sample_rate(ModTracker* tracker, uint32_t sampleRate){
	tracker->_updates_per_tick = sampleRate * 2.5 / tracker->bpm; 
	tracker->_sample_rate = sampleRate;
	printf("Set updates per tick to %d\n", tracker->_updates_per_tick);
}

void tracker_mod_update(ModTracker* tracker, int16_t* buffer, uint32_t buf_size){
	int samples_per_tick, buff_ptr;
	int16_t ch, samp_l, samp_r;
	Channel* chan;
	int16_t mixed;
	buff_ptr = 0;

	while(buff_ptr < buf_size){
		samp_l = 0;
		samp_r = 0;
		tracker_mod_tick(tracker);

#ifdef debug_write
		mixed = 0;
#endif

		for (ch = 0; ch < 4; ch++){
			chan = &tracker->channels[ch];
			if(chan->period == 0 || chan->volume == 0) continue;
			double freq = (((8363.0 * 428.0) / chan->period) / tracker->_sample_rate);
			if(tracker->module.sample_data[chan->instrument] == NULL) continue;
			
			int16_t sample = (int16_t)tracker->module.sample_data[chan->instrument][(uint32_t)chan->sample_offset];
			samp_l += (sample * chan->volume * (0xFF - chan->pan)) / 0xFF;
			samp_r += (sample * chan->volume * chan->pan) / 0xFF;

			chan->sample_offset += freq;

			MODSampleDef* instrument = &(tracker->module.samples[chan->instrument]);

			if(chan->sample_offset >= instrument->sample_length) { //(instrument->repeat_offset + instrument->repeat_length)
				if (instrument->repeat_length > 1)
				{
					//if loop length is more than 1, the sample loops (0 supposedly is unsupported but there's no docs)
					chan->sample_offset = instrument->repeat_offset + fmod(chan->sample_offset, instrument->repeat_length);
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