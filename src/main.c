#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "tracker.h"

static ModTracker* mod_tracker = NULL;

void audio_update(void* userdata, uint8_t* stream, int len){

	len /= sizeof(int16_t);
	tracker_mod_update(mod_tracker, (int16_t*)stream, (uint32_t)len);
}

int main(int argc, char* argv[]){

	mod_tracker = (ModTracker*)malloc(sizeof(ModTracker));

	SDL_Init(SDL_INIT_AUDIO);

	if(argc < 2) return 1;
	tracker_open_mod(mod_tracker, argv[1]);

	SDL_AudioSpec target_format;
	target_format.freq = 44100;
	target_format.format = AUDIO_S16;
	target_format.channels = 2;
	target_format.samples = 4096;
	target_format.callback = audio_update;
	target_format.userdata = NULL;

	SDL_AudioSpec device_format;
	SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &target_format, &device_format, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);

	if (dev < 0)
	{
		printf("Error initiating audio device!\nError %s\n", SDL_GetError());
		return 1;
	}

	tracker_mod_set_sample_rate(mod_tracker, 44100);

	//Let the audio device play
	SDL_PauseAudioDevice(dev, 0);

	uint8_t quit = 0;
	while(!quit){
		quit = (getchar() == 'q');
	}

	SDL_CloseAudioDevice(dev);
	tracker_close_mod(mod_tracker);

	free(mod_tracker);

	return 0;
}