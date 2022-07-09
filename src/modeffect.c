#include "modeffect.h"

//From FireLight Mod documentation
const uint8_t sine_table[32] = {
	   0, 24, 49, 74, 97,120,141,161,
	 180,197,212,224,235,244,250,253,
	 255,253,250,244,235,224,212,197,
	 180,161,141,120, 97, 74, 49, 24
};

const void(*effect_list[0x10])(ModTracker* tracker, Channel* chan) = 
{
	arpeggio,//0xy 
	porta_up,//1xx
	porta_down,//2xx
	tone_porta,//3xx
	vibrato,//4xx
	vol_slide_tone_porta,//5xy
	vol_slide_vibrato,//6xy
	tremolo,//7xy
	set_panning,//8xx - FT2 effect extension; not supported on protracker modules
	sample_offset,//9xx
	vol_slide,//Axy
	pos_jump,//Bxx
	set_volume,//Cxx
	pattern_break,//Dxx
	Exx_effect, //Exx
	set_speed_tempo//Fxx
};

void dummy_effect(ModTracker* tracker, Channel* chan)
{
	//dummyeffect does nothing

	chan->effect = 0;
}

void arpeggio(ModTracker* tracker, Channel* chan)
{
	//to do
	//printf("Pat %d - Row %d: Arpeggio not implemented\n", tracker->current_pattern, tracker->current_row);
}

void porta_up(ModTracker* tracker, Channel* chan)
{
	if(tracker->_current_ticks > 0 && chan->period > 133)
	{
		chan->period -= chan->effect_args;
		if(chan->period < 133) chan->period = 133;
	}
}

void porta_down(ModTracker* tracker, Channel* chan)
{
	if(tracker->_current_ticks > 0 && chan->period > 856)
	{
		chan->period += chan->effect_args;
		if(chan->period < 856) chan->period = 856;
	}
}

void tone_porta(ModTracker* tracker, Channel* chan)
{
	if(tracker->_current_ticks > 0 && chan->period != chan->porta_period)
	{
		if (chan->porta_period > chan->period)
		{
			chan->period += chan->effect_args;

			if (chan->period > chan->porta_period)
				chan->period = chan->porta_period;
		}
		else
		{
			chan->period -= chan->effect_args;

			if (chan->period < chan->porta_period)
				chan->period = chan->porta_period;
		}
	}
}

void vibrato(ModTracker* tracker, Channel* chan)
{
	//to do: implement more than just sine

	uint16_t vib = (sine_table[chan->vibrato_pos] * (chan->effect_args & 0x0F)) >> 7;
	chan->period = chan->note + (chan->vibrato_neg ? -vib : vib);
	chan->vibrato_pos += (chan->effect_args & 0xF);
	if(chan->vibrato_pos > 31){
		chan->vibrato_neg = !chan->vibrato_neg;
		chan->vibrato_pos = 0;
	}
}

void vol_slide_tone_porta(ModTracker* tracker, Channel* chan)
{
	vol_slide(tracker, chan);
	tone_porta(tracker, chan);
}

void vol_slide_vibrato(ModTracker* tracker, Channel* chan)
{
	vol_slide(tracker, chan);
	vibrato(tracker, chan);
}

void tremolo(ModTracker* tracker, Channel* chan)
{
	//to do
	printf("Pat %d - Row %d: tremolo not implemented\n", tracker->current_pattern, tracker->current_row);
}

//FT2 extension
void set_panning(ModTracker* tracker, Channel* chan)
{
	printf("set pan to %x\n", chan->effect_args);
	chan->pan = chan->effect_args;
	
	chan->effect = 0x00;
	chan->effect_args = 0x00;
}

void sample_offset(ModTracker* tracker, Channel* chan)
{
	chan->sample_offset = chan->effect_args * 0x100;

	//only do this once
	chan->effect = 0;
	chan->effect_args = 0;
}

void vol_slide(ModTracker* tracker, Channel* chan)
{
	if((chan->effect_args & 0xF0) >> 4 == 0)
	{
		chan->volume -= chan->effect_args & 0x0F;
	}
	else if((chan->effect_args & 0x0F) == 0)
	{
		chan->volume += (chan->effect_args & 0xF0) >> 4;
	}
}

void pos_jump(ModTracker* tracker, Channel* chan)
{
	tracker->current_pattern = chan->effect_args;
	if(tracker->current_pattern > tracker->module.song_length){
		tracker->current_pattern = tracker->module.song_length;
	}
}

void set_volume(ModTracker* tracker, Channel* chan)
{
	chan->volume = chan->effect_args;
}

void pattern_break(ModTracker* tracker, Channel* chan)
{
	if(tracker->_current_ticks == tracker->speed-1){
		tracker->current_pattern++;
		if(tracker->current_pattern > tracker->module.song_length){
			tracker->current_pattern = 0;
		}
		
		
		tracker->current_row = (((chan->effect_args & 0xF0) >> 4) * 10) + (chan->effect_args & 0x0F) - 1;
		if(tracker->current_row >= 64){
			tracker->current_row = 0;
		}
	}
}

void Exx_effect(ModTracker* tracker, Channel* chan)
{
	uint8_t effect_index = (chan->effect_args & 0xF0) >> 4;

	switch (effect_index)
	{
		case 0x01:
		{
			//E1x
			fine_porta_up(tracker, chan);
			break;
		}
		case 0x02:
		{
			//E2x
			fine_porta_down(tracker, chan);
			break;
		}
		case 0x04:
		{
			//E4x
			set_vib_waveform(tracker, chan);
			break;
		}
		case 0x05:
		{
			//E5x
			set_finetune(tracker, chan);
			break;
		}
		case 0x06:
		{
			//E6x
			pattern_loop(tracker, chan);
			break;
		}
		case 0x07:
		{
			//E7x
			set_tremolo_waveform(tracker, chan);
			break;
		}
		case 0x08:
		{
			//E8x
			set_panning_rough(tracker, chan);
			break;
		}
		case 0x09:
		{
			//E9x
			retrigger(tracker, chan);
			break;
		}
		case 0x0A:
		{
			//EAx
			fine_vol_slide_up(tracker, chan);
			break;
		}
		case 0x0B:
		{
			//EBx
			fine_vol_slide_down(tracker, chan);
			break;
		}
		case 0x0C:
		{
			//ECx
			note_cut(tracker, chan);
			break;
		}
		case 0x0D:
		{
			//EDx
			note_delay(tracker, chan);
			break;
		}
		case 0x0E:
		{
			//EEx
			pattern_delay(tracker, chan);
			break;
		}
		case 0x0F:
		{
			//EFx
			invert_loop(tracker, chan);
			break;
		}
		default:
		{
			break;
		}
	}
}

void fine_porta_up(ModTracker* tracker, Channel* chan)
{
	if(tracker->_current_ticks == 0 && chan->period > 133)
	{
		chan->period -= chan->effect_args;
		if(chan->period < 133) chan->period = 133;
	}
}

void fine_porta_down(ModTracker* tracker, Channel* chan)
{
	if(tracker->_current_ticks == 0 && chan->period > 856)
	{
		chan->period += chan->effect_args;
		if(chan->period < 856) chan->period = 856;
	}
}

void set_vib_waveform(ModTracker* tracker, Channel* chan)
{
	//to do
	printf("Pat %d - Row %d: Set Vibrato Waveform not implemented\n", tracker->current_pattern, tracker->current_row);
}

void set_finetune(ModTracker* tracker, Channel* chan)
{
	int8_t finetune = (chan->effect_args & 0x0F);
	tracker->module.samples[chan->instrument].finetune = (finetune > 7 ? finetune - 16 : finetune);
}

void pattern_loop(ModTracker* tracker, Channel* chan)
{
	if (chan->effect_args & 0x0F == 0x00)
	{
		//marks the start of the loop
		tracker->loop_row = tracker->current_row;
		tracker->loop_count = 0;
	}
	else
	{
		//if we are on the final tick, we swap rows
		if (tracker->_current_ticks == tracker->speed && tracker->loop_count < chan->effect_args & 0x0F)
		{
			tracker->current_row = tracker->loop_row;
			tracker->loop_count++;
		}
	}
}

void set_tremolo_waveform(ModTracker* tracker, Channel* chan)
{
	//to do
	printf("Pat %d - Row %d: Set Tremolo Waveform not implemented\n", tracker->current_pattern, tracker->current_row);
}

//FT2 extension
void set_panning_rough(ModTracker* tracker, Channel* chan)
{
	printf("set pan (rough) to %u\n", chan->effect_args & 0x0F);
	chan->pan = (chan->effect_args & 0x0F) * 0x0F;
	
	chan->effect = 0x00;
	chan->effect_args = 0x00;
}

void retrigger(ModTracker* tracker, Channel* chan)
{
	if((tracker->_current_ticks % (chan->effect_args & 0x0F)) == 0){
		chan->sample_offset = 0;
	}
}

void fine_vol_slide_up(ModTracker* tracker, Channel* chan)
{
	if (tracker->_current_ticks == 0)
	{
		chan->volume += chan->effect_args & 0x0F;

		if (chan->volume > 64)
			chan->volume = 64;
	}
}

void fine_vol_slide_down(ModTracker* tracker, Channel* chan)
{
	if (tracker->_current_ticks == 0)
	{
		chan->volume -= chan->effect_args & 0x0F;

		if (chan->volume < 0)
			chan->volume = 0;
	}
}

void note_cut(ModTracker* tracker, Channel* chan)
{
	if (tracker->_current_ticks == (chan->effect_args & 0x0F))
	{
		chan->period = 0;
		chan->sample_offset = 0;

		//only do this once
		chan->effect = 0;
		chan->effect_args = 0;
	}
}

void note_delay(ModTracker* tracker, Channel* chan)
{
	printf("note delay\n");
	if(tracker->_current_ticks < chan->effect_args & 0x0F){
		chan->period = 0;
	} else if(tracker->_current_ticks == chan->effect_args & 0x0F){
		chan->period = chan->note;
	}
}

void pattern_delay(ModTracker* tracker, Channel* chan)
{
	//to do
	printf("Pat %d - Row %d: pattern delay not implemented\n", tracker->current_pattern, tracker->current_row);
}

void invert_loop(ModTracker* tracker, Channel* chan)
{
	//to do
	printf("Pat %d - Row %d: invert loop not implemented\n", tracker->current_pattern, tracker->current_row);
}

void set_speed_tempo(ModTracker* tracker, Channel* chan)
{
	//printf("set tempo/speed\n");
	if(tracker->_current_ticks == 0){
		if(chan->effect_args <= 0x1F && chan->effect_args != 0x00){
			tracker->speed = chan->effect_args;
		} else if(chan->effect_args >= 0x20 && chan->effect_args <= 0xFF){
			tracker->bpm = chan->effect_args;
			tracker->_updates_per_tick = tracker->_sample_rate * 2.5 / (chan->effect_args);
		}


		//only do this once
		chan->effect = 0;
		chan->effect_args = 0;
	}
}