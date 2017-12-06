#ifndef __CHANNEL_H__
#define __CHANNEL_H__
#include "song.h"
struct ChannelState {
  Instrument *inst; //!< the instrument being played
  Sample *smp;      //!< the sample being played.
  unsigned no;      //!< the channel number (for registers and stuff).
  Cell cell;        //!< a copy of the pattern's cell
  Cell *current_cell;  //!< for free-riding track, pointer to the cell.
  u16 srcsmp_cache; // see playNote/bendNote.
  u8 note, lastparam;

  // -=- effect-specific variables -=-
  s16 actual_finetune;                   // for Exx, Fxx and Hxx
  s16 vibpos, vibdepth, vibspeed;        // for Hxx
  s16 slidetune_speed;  u8 target_note;  // for Gxx

  ChannelState(int no=0) : 
    inst(0), smp(0), no(no), cell(), current_cell(0), srcsmp_cache(),
    note(EMPTY_NOTE), lastparam(NO_EFFECT_PARAM),
    actual_finetune(0), vibpos(0), vibdepth(0), vibspeed(0),
    slidetune_speed(0), target_note(EMPTY_NOTE)
  {
    cell = { EMPTY_NOTE,NO_INSTRUMENT,0,
	     NO_EFFECT, NO_EFFECT_PARAM,
	     NO_EFFECT, NO_EFFECT_PARAM,
    };
  }
};

#endif
