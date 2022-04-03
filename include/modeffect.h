#ifndef __MODEFFECT_H__
#define __MODEFFECT_H__

#include "tracker.h"

//Effect subroutine declarations - all in format void (Channel*)
extern void dummy_effect(ModTracker* tracker, Channel* chan); //N/A
extern void arpeggio(ModTracker* tracker, Channel* chan); //0xy 
extern void porta_up(ModTracker* tracker, Channel* chan); //1xx
extern void porta_down(ModTracker* tracker, Channel* chan); //2xx
extern void tone_porta(ModTracker* tracker, Channel* chan); //3xx
extern void vibrato(ModTracker* tracker, Channel* chan); //4xx
extern void vol_slide_tone_porta(ModTracker* tracker, Channel* chan); //5xy
extern void vol_slide_vibrato(ModTracker* tracker, Channel* chan); //6xy
extern void tremolo(ModTracker* tracker, Channel* chan); //7xy
extern void set_panning(ModTracker* tracker, Channel* chan); //8xx - FT2 effect extension; not supported on protracker modules
extern void sample_offset(ModTracker* tracker, Channel* chan); //9xx
extern void vol_slide(ModTracker* tracker, Channel* chan); //Axy
extern void pos_jump(ModTracker* tracker, Channel* chan); //Bxx
extern void set_volume(ModTracker* tracker, Channel* chan); //Cxx
extern void pattern_break(ModTracker* tracker, Channel* chan); //Dxx
//extern void SetFilter(ModTracker* tracker, Channel* chan); //E0x - original Amiga effect; may not be needed?
extern void fine_porta_up(ModTracker* tracker, Channel* chan); //E1x
extern void fine_porta_down(ModTracker* tracker, Channel* chan); //E2x
//extern void GlissandoConyrol(ModTracker* tracker, Channel* chan); //E3x - effect is poorly documented and usually not supported
extern void set_vib_waveform(ModTracker* tracker, Channel* chan); //E4x
extern void set_finetune(ModTracker* tracker, Channel* chan); //E5x
extern void pattern_loop(ModTracker* tracker, Channel* chan); //E6x - 0 sets loop start, x jumps to the start point x times
extern void set_tremolo_waveform(ModTracker* tracker, Channel* chan); //E7x
extern void set_panning_rough(ModTracker* tracker, Channel* chan); //E8x - Another FT2 extension
extern void retrigger(ModTracker* tracker, Channel* chan); //E9x
extern void fine_vol_slide_up(ModTracker* tracker, Channel* chan); //EAx
extern void fine_vol_slide_down(ModTracker* tracker, Channel* chan); //EBx
extern void note_cut(ModTracker* tracker, Channel* chan); //ECx
extern void note_delay(ModTracker* tracker, Channel* chan); //EDx
extern void pattern_delay(ModTracker* tracker, Channel* chan); //EEx
extern void invert_loop(ModTracker* tracker, Channel* chan); //EFx
extern void set_speed_tempo(ModTracker* tracker, Channel* chan); //Fxx

extern const void(*effect_list[0xFF])(ModTracker* tracker, Channel* chan);

#endif //!__MODEFFECT__