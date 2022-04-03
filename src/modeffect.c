#include "modeffect.h"

static const void(*effect_list[0xFF])(ModTracker* tracker, Channel* chan) = 
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
    dummy_effect, //SetFilter,//E0x - original Amiga effect; may not be needed?
    fine_porta_up,//E1x
    fine_porta_down,//E2x
    dummy_effect, //GlissandoConyrol,//E3x - effect is poorly documented and usually not supported
    set_vib_waveform,//E4x
    set_finetune,//E5x
    pattern_loop,//E6x - 0 sets loop start, x jumps to the start point x times
    set_tremolo_waveform,//E7x
    set_panning_rough,//E8x - Another FT2 extension
    retrigger,//E9x
    fine_vol_slide_up,//EAx
    fine_vol_slide_down,//EBx
    note_cut,//ECx
    note_delay,//EDx
    pattern_delay,//EEx
    invert_loop,//EFx
    set_speed_tempo,//Fxx
};

void dummy_effect(ModTracker* tracker, Channel* chan)
{
    //dummyeffect does nothing

    chan->effect = 0;
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
            chan->period += chan->effect_args;
        else
            chan->period -= chan->effect_args;
    }
}

void sample_offset(ModTracker* tracker, Channel* chan)
{
    chan->sample_offset = chan->effect_args * 0x100;

    //only do this once
    chan->effect = 0;
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

void set_volume(ModTracker* tracker, Channel* chan)
{
    chan->volume = chan->effect_args & 64;
}