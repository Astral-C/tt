#include "tracker.h"
#include <stdio.h>
#include <stdlib.h>

uint8_t tracker_open_mod(ModTracker* tracker, char* mod){
	FILE* mod_file;
	int i;

	if(tracker == NULL) return 1; //oops

	mod_file = fopen(mod, "r");
	fread(&tracker->module.module_name, sizeof(tracker->module.module_name), 1, mod_file);
	fread(&tracker->module.samples, sizeof(tracker->module.samples), 1, mod_file);

	fread(&tracker->module.song_length, sizeof(tracker->module.song_length), 1, mod_file); //length in patterns
	fread(&tracker->module.old_tracker_force, sizeof(tracker->module.old_tracker_force), 1, mod_file);
	fread(&tracker->module.song_positions, sizeof(tracker->module.song_positions), 1, mod_file);
	fread(&tracker->module.signature, sizeof(tracker->module.signature), 1, mod_file);

	printf("Module name is %s\n", tracker->module.module_name);
	
	
	for(i = 0; i < 128; i++){
		if(tracker->module.song_positions[i] > tracker->module.pattern_count){
			tracker->module.pattern_count = tracker->module.song_positions[i];
		}
	}

	printf("# of patterns: %u\n", tracker->module.pattern_count);

	tracker->module.patterns = malloc(sizeof(MODPattern) * tracker->module.pattern_count);
	
	fread(&tracker->module.patterns[0], sizeof(MODPattern), tracker->module.pattern_count, mod_file);

	//read sample data
	for(i = 0; i < 32; i++){
		MODSampleDef* def = &tracker->module.samples[i];
		def->sample_length <<= 1;
		def->repeat_offset <<= 1;
		def->repeat_length <<= 1;

		//only read if the sample len is > 0
		if(def->sample_length == 0) continue;

		tracker->module.sample_data[i] = malloc(def->sample_length);
		fread(tracker->module.sample_data[i], def->sample_length, 1, mod_file);
	}
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

}