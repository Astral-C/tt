#include <stdio.h>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "tracker.h"

void update_player(ma_device* device, void* output, const void* input, ma_uint32 frame_count){
	ModTracker* tracker;
	tracker = (ModTracker*)device->pUserData;

	tracker_mod_update(tracker, output, frame_count);

}

int main(int argc, char* argv[]){
	ModTracker tracker;

	if(argc < 2) return 1;
	tracker_open_mod(&tracker, argv[1]);

	ma_device_config config = ma_device_config_init(ma_device_type_playback);
	config.playback.format = ma_format_s16;
	config.playback.channels = 2;
	config.sampleRate = 44100;
	config.dataCallback = update_player;
	config.pUserData = &tracker; //attach the tracker to the device as its userdata

	tracker_mod_set_sample_rate(&tracker, config.sampleRate);

	ma_device dev;
	if(ma_device_init(NULL, &config, &dev) != MA_SUCCESS){
		printf("Failed to open device\n");
		return -1;
	}

	ma_device_start(&dev);

	uint8_t quit = 0;
	while(!quit){
		quit = (getchar() == 'q');
	}

	ma_device_uninit(&dev);
	tracker_close_mod(&tracker);

	return 0;
}
