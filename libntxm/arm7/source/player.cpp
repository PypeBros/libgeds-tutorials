// libNTXM - XM Player Library for the Nintendo DS
// Copyright (C) 2005-2007 Tobias Weyand (0xtob)
//                         me@nitrotracker.tobw.net
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include "ntxm/player.h"

#include "ntxm/song.h"
extern "C" {
  #include "demokit.h"
}
#include "tools.h"
#include "command.h"
#include <string.h> // for memcpy
#include <stdlib.h>

#define MIN(x,y)	((x)<(y)?(x):(y))
#ifdef __PARANOIA__
#define RGB15(r,g,b)  ((r)|((g)<<5)|((b)<<10))
#define RGB_DEBUG(r,g,b) IPC->battery=RGB15(r,g,b)
#else
#define RGB_DEBUG(r,g,b)
#endif

// well, we don't plan to have multiple simultaneous players, do we ?
PlayerState Player::state;
EffectState Player::effstate;
struct ChannelState Player::chan[MAX_CHANNELS];


// straight out of libmikmod (LGPL)
static u8 VibratoTable[32]={
	  0, 24, 49, 74, 97,120,141,161,180,197,212,224,235,244,250,253,
	255,253,250,244,235,224,212,197,180,161,141,120, 97, 74, 49, 24
};


/* ===================== PUBLIC ===================== */

Player::Player(void (*_externalTimerHandler)(void))
  :NtMagic("NPYR"), song(0),externalTimerHandler(_externalTimerHandler),
   onRow(0), onPatternChange(0), onSampleFinish(0), lastms(0)
{
  //  song=0;
  initState();
  initEffState();
  
  demoInit();
  
  // Init callbacks
  /* onRow = 0;
     onPatternChange = 0;
     onSampleFinish = 0;
  */
  startPlayTimer();
}

void* Player::operator new (size_t size) {
 
	return malloc(size);
 
} // default ctor implicitly called here
 
void Player::operator delete (void *p) {
 
	if ( NULL != p ) free(p);
 
} // default dtor implicitly called here

void Player::setSong(Song *_song)
{
  song = _song;
  initState();
	
  // Init fading
  my_memset(state.channel_fade_active, 0, sizeof(state.channel_fade_active));
  my_memset(state.channel_fade_ms, 0, sizeof(state.channel_fade_ms));
  my_memset(state.channel_volume, 0, sizeof(state.channel_volume));
  my_memset(state.channel_fade_target_volume, 0, sizeof(state.channel_fade_target_volume));
}

// Set the current pattern to looping
void Player::setPatternLoop(bool loopstate) {
  state.patternloop = loopstate;
}

/** cells content on pattern @pno from given @row, channels 1-@nchns
 * will be playing from @chn
 */
void Player::playSfx(u8 pno, u8 row, u8 chn, u8 nchns)
{
  if (pno >= song->n_patterns)
    return;
  if (chn + nchns > MAX_CHANNELS)
    return;
  if (nchns > song->n_channels)
    return;
  if (row >= song->patternlengths[pno])
    return;
  Cell** pattern = song->patterns[pno];
  for (u8 i=0; i < nchns; i++) {
    chan[chn + i].current_cell = pattern[i] + row;
  }
}

// Plays the song till the end starting at pattern order table position potpos and row row
void Player::play(u8 potpos, u16 row, bool repeat)
{

  // Mark all channels inactive
  if(state.playing == false) {
    my_memset(state.channel_active, 0, sizeof(state.channel_active));
    my_memset(state.channel_ms_left, 0, sizeof(state.channel_ms_left));
    my_memset(state.channel_loop, 0, sizeof(state.channel_loop));
  } else {
    for(u8 chn = 0 ; chn < MAX_CHANNELS; chn++)
      {
	if (chn >= song->n_channels && chan[chn].current_cell==0) continue;
	state.channel_fade_active[chn] = 1;
	state.channel_fade_ms[chn] = FADE_OUT_MS;
	state.channel_fade_target_volume[chn] = 0;
      }
  }
  state.potpos = potpos;
  state.row = row;
  state.pattern = song->pattern_order_table[state.potpos];
  state.songloop = repeat;
  
  // Reset ms and tick counter
  state.tick_ms = 0;
  state.row_ticks = 0;
  
  state.juststarted = true;
  
  lastms = getTicks();
  
  initEffState();
  
  state.playing = true;
}

void Player::stop(void)
{
  state.playing = false;
  
  // Stop all playing samples
  
  for(u8 chn = 0; chn < MAX_CHANNELS; chn++)
    {
      if (chn >= song->n_channels && chan[chn].current_cell==0) continue;

      state.channel_fade_active[chn] = 1;
      state.channel_fade_ms[chn] = FADE_OUT_MS;
      state.channel_fade_target_volume[chn] = 0;
    }
}

// Play the note with the given settings
/* note that we don't need to fade in to the appropriate volume.
 * only fading out from the previous one is required.
 */
void Player::playNote(u8 note, u8 volume, ChannelState* chn, u8 instidx)
{
  u8 channel=chn->no;
  if( (state.playing == true) && (song->channelMuted(channel) == true) )
    return;
  
  // Stop possibly active fades
  state.channel_fade_active[channel] = 0;
  state.channel_fade_ms[channel] = 0;
  //state.channel_instrument[channel] = instidx;
  Instrument *inst = chan[channel].inst; //song->instruments[instidx];
  
  if(volume == NO_VOLUME) {
    state.channel_volume[channel] = MAX_VOLUME * inst->getSampleForNote(note)->getVolume() / 255;
  } else {
    state.channel_volume[channel] = volume * inst->getSampleForNote(note)->getVolume() / 255;
  }
  
  state.channel_fade_vol[channel] = state.channel_volume[channel];
  
  state.channel_note[channel]   = note;
  state.channel_active[channel] = 1;
  
  if(inst != 0)
    {
      //CommandDbgOut("a7: play %d %d %u\n",instidx, note, volume);
      inst->play(note, volume, chn);
      if (chan[channel].smp==0)
	song->setoops(OOPS_INST_WO_SMP,state.pattern, state.row, channel);
    }
}


// Play the given sample (and send a notification when done)
void Player::playSample(Sample *sample, u8 note, u8 volume, u8 channel)
{
	// Stop playing sample if necessary
	if(state.playing_single_sample == true) {
		state.playing_single_sample = false;
		state.single_sample_ms_remaining = 0;
		CommandSampleFinish();
	}
	
	// Calculate length
	u32 length = sample->calcPlayLength(note);
	
	// Set sample playing state
	state.playing_single_sample = true;
	state.single_sample_ms_remaining = length;
	state.single_sample_channel = 0;
	
	// Play
	sample->play(note, volume, chan+channel);
}

// Stop playback on a channel
void Player::stopChannel(u8 channel)
{
	state.channel_fade_active[channel]        = 1;
	state.channel_fade_ms[channel]            = FADE_OUT_MS;
	state.channel_fade_target_volume[channel] = 0;
	//state.channel_note[channel]               = EMPTY_NOTE;
	//state.channel_instrument[channel]         = NO_INSTRUMENT;
	
	// Stop single sample if it's played on this channel
	if((state.playing_single_sample == true) && (state.single_sample_channel == channel))
	{
		state.playing_single_sample = 0;
		state.single_sample_ms_remaining = 0;
		CommandSampleFinish();
	}
}

void Player::registerRowCallback(void (*onRow_)(u16)) {
	onRow = onRow_;
}

void Player::registerPatternChangeCallback(void (*onPatternChange_)(u8)) {
	onPatternChange = onPatternChange_;
}

void Player::registerSampleFinishCallback(void (*onSampleFinish_)()) {
	onSampleFinish = onSampleFinish_;
}




void Player::playTimerHandler(void)
{
  u32 passed_time = getTicks() - lastms;
  lastms = getTicks();
	
  // Fading stuff
  handleFade(passed_time);
	
  // Are we playing a single sample (a sample not from the song)?
  // (Built in for games etc)
  if(state.playing_single_sample)
    {
      // Count down, and send signal when done
      if(state.single_sample_ms_remaining < passed_time)
	{
	  state.playing_single_sample = false;
	  state.single_sample_ms_remaining = 0;
	  state.single_sample_channel = 0;
			
	  if(onSampleFinish != 0) {
	    onSampleFinish();
	  }
	}
      else
	{
	  state.single_sample_ms_remaining -= passed_time;
	}
    }
	
  // Update tick ms
  state.tick_ms += passed_time;
	
  if (!song) {
    static int frames=0;
//    RGB_DEBUG(31,0,(frames++)&1); 
    return;
  }
  RGB_DEBUG(0,0,24);

  if(state.tick_ms >= song->getMsPerTick() - FADE_OUT_MS)
    {
      // we are shortly before the next tick. (As long as a fade would take)
      /* NDS sound hardware doesn't allow us to alter the volume directly 
       * (if we do, it produce sound glitches), so we have to perform fading
       * from the current volume value to the next one, requiring some actions
       * to take place ahead of time. 
       */

      // Is there a request to set the volume?
      /* effstate.channel_setvol_requested is altered when EFFECT_SET_VOLUME (Cxx),
       * EFFECT_VOLUME_SLIDE (Axx) or EFFECT_E_NOTE_CUT (ECx) is used, 
       * state.channel_fade_target_volume being altered jointly.
       * (.MOD effects numbering applied)
       */

      for(u8 channel=0; channel<MAX_CHANNELS; ++channel)
	{
	  if (channel >= song->n_channels && chan[channel].current_cell==0) continue;

	  if(effstate.channel_setvol_requested[channel])
	    {
	      state.channel_fade_active[channel] = 1;
	      state.channel_fade_ms[channel] = FADE_OUT_MS;
	      effstate.channel_setvol_requested[channel] = false;
	    }
	}
      /* (?) does .MOD has a "volume column" at all ? 
       * http://upload.wikimedia.org/wikipedia/fi/b/b7/Protracker-2.3d.png suggest not
       * as well as the presence of "set volume" command.
       
       Info for each note:     
       <blockquote><pre><tt>
       _____byte 1_____   byte2_    _____byte 3_____   byte4_
       /                 /        /                 /
       0000          0000-00000000  0000          0000-00000000
       
       Upper four    12 bits for    Lower four    Effect command.
       bits of sam-  note period.   bits of sam-
       ple number.                  ple number.
       </tt></pre></blockquote>
       ==> .MOD is 16 effects, no separate volume command, 16 or 32 instruments.
       */
      
		
      // Is this the last tick before the next row?
      if(state.row_ticks >= song->getTempo()-1)
	{
	  // If so, check if for any of the active channels a new note starts in the next row. 
	  /* Again, we need to fade out the current note before the new note take
	   * place, to avoid sound glitches. "PORTA_TO_NOTE" inhibit this, as rather
	   * than having a *new* note played, it keeps running the current note, but
	   * use the "note" column as effect argument.
	   */

	  u8 nextNote;
	  for(u8 channel=0; channel<MAX_CHANNELS; ++channel)
	    {
	      if (channel >= song->n_channels && chan[channel].current_cell==0) continue;
	      if(state.channel_active[channel] == 1)
		{
		  u16 nextrow;
		  u8 nextpattern, nextpotpos;
					
		  calcNextPos(&nextrow, &nextpotpos);
		  nextpattern = song->pattern_order_table[nextpotpos];
			
		  if (song->patterns[nextpattern][channel][nextrow].effect!=
		      EFFECT_PORTA_NOTE)
		    nextNote = song->patterns[nextpattern][channel][nextrow].note;
		  else nextNote=EMPTY_NOTE;

		  if((nextNote!=EMPTY_NOTE) && (state.channel_fade_active[channel] == 0))
		    {
		      // If so, fade out to avoid a click.
		      state.channel_fade_active[channel] = 1;
		      state.channel_fade_ms[channel] = FADE_OUT_MS;
		      state.channel_fade_target_volume[channel] = 0;
		    }
		}
	    }
	}
    }
  RGB_DEBUG(0,0,25);

  // Update active channels
  for(u8 channel=0; channel<MAX_CHANNELS; ++channel)
    {
      if (channel >= song->n_channels && chan[channel].current_cell==0) continue;

      if(state.channel_ms_left[channel] > 0)
	{
	  if(state.channel_ms_left[channel] > passed_time) {
	    state.channel_ms_left[channel] -= passed_time;
	  } else {
	    state.channel_ms_left[channel] = 0;
	  }
		
	  if((state.channel_ms_left[channel]==0)&&(state.channel_loop[channel]==false)) {
	    state.channel_active[channel] = 0;
	  }
	}
    }
  RGB_DEBUG(0,0,26);
	
  // Update envelopes
  for(u8 channel=0; channel<MAX_CHANNELS; ++channel)
    {
      if (channel >= song->n_channels && chan[channel].current_cell==0) continue;

      if(state.channel_active[channel])	{
	Instrument *inst=chan[channel].inst;
	inst->updateEnvelopePos(song->getBPM(), passed_time);
	state.channel_env_vol[channel] = inst->getEnvelopeAmp();
      }
    }
  RGB_DEBUG(0,0,27);

  // Update channel volumes
  for(u8 channel=0; channel<MAX_CHANNELS; ++channel)
    {
	if (channel >= song->n_channels && chan[channel].current_cell==0) continue;

      if(state.channel_active[channel])
	{
	  // The master formula!
	  // FIXME: Use inst volume too!
	  u8 chnvol = state.channel_env_vol[channel] * state.channel_fade_vol[channel] / 64;
			
	  SCHANNEL_VOL(channel) = SOUND_VOL(chnvol);
	}
    }
  RGB_DEBUG(0,0,28);
	
  if(state.playing == false)
    return;
		
  if( state.juststarted == true )  {
    state.juststarted = false;
    
    playRow();
    handleEffects();
    handleTickEffects();
		
    if(onRow != NULL) {
      onRow(state.row);
    }
  }
  RGB_DEBUG(0,0,29);
	
  // if the number of ms per tick is reached, go to the next tick
  if(state.tick_ms >= song->getMsPerTick())
    {
      // Go to the next tick
      state.row_ticks++;
		
      if(state.row_ticks >= song->getTempo()) {
	// the row has been fully played. Let's start a new one.
	state.row_ticks = 0;
	
	bool finished = calcNextPos(&state.row, &state.potpos);
	if(finished == true) {
	  stop();
	} else {
	  state.pattern = song->pattern_order_table[state.potpos];
	}
			
	if(state.waitrow == true) {
	  stop();
	  CommandNotifyStop();
	  state.waitrow = false;
	  return;
	}
			
	if((effstate.pattern_break_requested == true) && (onPatternChange!=NULL)) {
	  onPatternChange(state.potpos);
	}
	finishEffects();
	playRow();
	handleEffects();
	
	if(onRow != NULL) {
	  onRow(state.row);
	}
	song->setpos(state.potpos, state.row);			
	if( (state.row == 0) && (onPatternChange != 0) ) {
	  onPatternChange(state.potpos);
	}
      }

      // happens at every tick, new row or not.
      handleTickEffects();
      state.tick_ms -= song->getMsPerTick();
    }
  RGB_DEBUG(0,0,30);

}

/* ===================== PRIVATE ===================== */

void Player::startPlayTimer(void)
{
  // we just set up the frequency here. Actual interrupt handler registering
  // is the job of the ARM7 template, and our play routine will be called 
  // through NTXM7::timerHandler().
  TIMER0_DATA = TIMER_FREQ_64(1000); // Call handler every millisecond
  TIMER0_CR = TIMER_ENABLE | TIMER_IRQ_REQ | TIMER_DIV_64;
}

void Player::playRow(void)
{
  // Play all notes in this row
  for(u8 channel=0; channel<MAX_CHANNELS; ++channel)
    {
      if (channel >= song->n_channels && chan[channel].current_cell==0) continue;

      ChannelState *chn=chan+channel;
      if (chn->current_cell)
	memcpy(&(chn->cell), chn->current_cell++, sizeof(Cell));
      else 
	memcpy(&(chn->cell),&(song->patterns[state.pattern][channel][state.row]),
	       sizeof(Cell));
      u8 note=chn->cell.note;
      if (chn->cell.effect_param == NO_EFFECT_PARAM)
	chn->cell.effect_param=chn->lastparam;
      else
	chn->lastparam=chn->cell.effect_param;
      
      if((note==EMPTY_NOTE)||(note==STOP_NOTE)||
	 (chn->cell.instrument==NO_INSTRUMENT))
	continue;
      
      if (chn->cell.effect==EFFECT_PORTA_NOTE) {
	// these effects do not *play* a new note: they define a new target
	// for the current note.
	if (chn->inst != song->instruments[chn->cell.instrument])
	  song->setoops(OOPS_SLIDE_CHANGE_INST,state.pattern,state.row,channel);

	chn->inst=song->instruments[chn->cell.instrument];
	chn->target_note = chn->cell.note;


	if (chn->target_note > chn->note) {
	  // current note is too low. bend it up.
	  chn->slidetune_speed=chn->cell.effect_param;
	} else {
	  // current note is too high. bend it down.
	  chn->slidetune_speed=-chn->cell.effect_param;
	}
	song->setoops(OOPS_XP_PORTA_NOTE_SETUP^0xC000,state.pattern,state.row,channel);

      } else {
	// a new note is here. flush the "caches".
	chn->actual_finetune=0;
	chn->note=note;
	chn->inst=song->instruments[chn->cell.instrument];
	if (chn->cell.volume==0)
	  song->setoops(OOPS_NOTE_MUTED,state.pattern,state.row,channel);
	
	// now play the note.
	playNote(note, chn->cell.volume, chn, chn->cell.instrument);
	
	state.channel_active[channel] = 1;
	if(chn->inst->getSampleForNote(note)->getLoop() != 0) {
	  song->setoops(OOPS_EXPERIMENTS+42,state.pattern,state.row,channel);
	  state.channel_loop[channel] = true;
	  state.channel_ms_left[channel] = 0;
	} else {
	  state.channel_loop[channel] = false;
	  state.channel_ms_left[channel] = chn->inst->calcPlayLength(note);
	}
      }
    }
}

void Player::playInstrument(u32 instr, u32 note, u32 volume, u32 channel) {
  ChannelState *chn=chan+channel;

  // a new note is here. flush the "caches".
  chn->actual_finetune=0;
  chn->note=note;
  chn->inst=song->instruments[instr];

  // now play the note.
  playNote(note, volume, chn, instr);
  
  state.channel_active[channel] = 1;
  if(chn->inst->getSampleForNote(note)->getLoop() != 0) {
    song->setoops(OOPS_EXPERIMENTS+42,state.pattern,state.row,channel);
    state.channel_loop[channel] = true;
    state.channel_ms_left[channel] = 0;
  } else {
    state.channel_loop[channel] = false;
    state.channel_ms_left[channel] = chn->inst->calcPlayLength(note);
  }
}



// supported so far : 
// vol brk setloop.
/*! when a new row starts, this function is called *before* 
 *  handleTickEffects, so it is safe to do any "prepare state"
 *  processing here, and keep TickEffects slim and sexy.
 */
void Player::handleEffects(void)
{
  effstate.pattern_loop_jump_now = false;
  effstate.pattern_break_requested = false;
  effstate.pattern_jump_requested = false;
  
  for(u8 i=0; i<MAX_CHANNELS; ++i)  {
    if (i >= song->n_channels && chan[i].current_cell==0) continue;

    ChannelState *chn=chan+i;
    for (uint j=0; j<2; j++) {
      u8 effect = (j==0)?chn->cell.effect:chn->cell.effect2;
      u8 param  = (j==0)?chn->cell.effect_param:chn->cell.effect2_param;
    
      if(effect != NO_EFFECT) {
	switch(effect) {
	  /* note has already been played at this point. */
	  
	case EFFECT_VIBRATO:
	  if (param&0x0f) chn->vibdepth=param&0x0f;
	  if (param&0xf0) chn->vibspeed=(param&0xf0)>>2;
	  break;
	  
	case EFFECT_9_OFFSET: 
	  chn->smp->advance(0|(param<<8),chan+i);
	  // TODO:      ^ some player can change the "hi-offset"
	  break;
	  
	case EFFECT_F_TEMPO_SPEED:
	  if (param<0x20) song->setTempo(param);
	  else song->setBpm(param);
	  break;
	  
	case(EFFECT_E): // If the effect is E, the effect type is specified in the 1st param nibble
	  {
	    u8 e_effect_type  = (param >> 4);
	    u8 e_effect_param = (param & 0x0F);
	    
	    switch(e_effect_type)	    {
	    case(EFFECT_E_SET_LOOP):
	      {
		// If param is 0, the loop start is set at the current row.
		// If param is >0, the loop end is set at the current row and
		// the effect param is the loop count
		if(e_effect_param == 0){
		  effstate.pattern_loop_begin = state.row;
		} else{
		  if(effstate.pattern_loop_count > 0)  {
		    // If we are already looping
		    effstate.pattern_loop_count--;
		    if(effstate.pattern_loop_count == 0) {
		      effstate.pattern_loop_begin = 0;
		    }
		  } else {
		    effstate.pattern_loop_count = e_effect_param;
		  }
		  
		  if(effstate.pattern_loop_count > 0) {
		    effstate.pattern_loop_jump_now = true;
		  }
		}
		break;
	      }
	    }
	    
	    break;
	  }
	case EFFECT_SET_VOLUME:	{
	  u8 target_volume = MIN(MAX_VOLUME, param * 2);
	  
	  // Request volume chnge and set target volume
	  effstate.channel_setvol_requested[i] = true;
	  state.channel_fade_target_volume[i] = target_volume;
	  
	  break;
	}
	  
	case EFFECT_PATTERN_BREAK: {
	  if (chn->current_cell) {
	    chn->current_cell = 0;
	    break;
	  }
	  // The row at which the next pattern is continued
	  // is <s>calculated strangely</s> BCD-encoded:
	  u8 b1, b2, newrow;
	  b1 = param >> 4;
	  b2 = param & 0x0F;
	  
	  newrow = b1 * 10 + b2;
	  
	  effstate.pattern_break_requested = true;
	  effstate.pattern_break_row = newrow;
	  
	  
	  break;
	}

        case EFFECT_ORDER_JUMP: {
	  effstate.pattern_jump_requested = true;
	  effstate.pattern_jump_order = param<song->getPotLength()?param:0;
	  break;
	}
	} // end of switch
      }   // end of if(effect)
    }     // end of effect/volume effect loop
  }       // end of channel loop
}         // return

// supported so far : 
// arp vsl notecut
void Player::handleTickEffects(void)
{
  for(u8 channel=0; channel<MAX_CHANNELS; ++channel) {
    if (channel >= song->n_channels && chan[channel].current_cell==0) continue;

    ChannelState *chn=chan+channel;
    u8 effect=chn->cell.effect;
    
    if(effect != NO_EFFECT) {
      u8 param = chn->cell.effect_param;

      /*      if (param == NO_EFFECT_PARAM)
	      param=chn->lastparam;
	      else
	      chn->lastparam=param;
      */
      
      switch(effect) {
      case(EFFECT_ARPEGGIO): {
	// this one will sound weird if not called on row #0 
	u8 halftone1, halftone2;
	halftone2 = (param & 0xF0) >> 4;
	halftone1 = param & 0x0F;
	
	if (chn->inst == 0)
	  song->setoops(OOPS_EFF_WO_INST,state.pattern,state.row,channel);
	if (chn->smp == 0) 
	  song->setoops(OOPS_EFF_WO_SMP,state.pattern,state.row,channel);
	if (chn->smp == 0) 
	  continue;
	
	switch(state.row_ticks % 3) {
	case(0): chn->smp->bendNote(chn->note + 0, 0, chn);  break;
	case(1): chn->smp->bendNote(chn->note + halftone1, 0, chn); break;
	case(2): chn->smp->bendNote(chn->note + halftone2, 0, chn); break;
	}
	
	break;
      }
				
      case(EFFECT_E): // If the effect is E, the effect type is specified in the 1st param nibble
	{
	  u8 e_effect_type  = (param >> 4);
	  u8 e_effect_param = (param & 0x0F);
	  
	  switch(e_effect_type) {
	  case(EFFECT_E_NOTE_CUT): {
	    if(e_effect_param == state.row_ticks) {
	      effstate.channel_setvol_requested[channel] = true;
	      state.channel_fade_target_volume[channel] = 0;
	    }
	    break;
	  }
	  }
	  break;
	}
      case(EFFECT_PORTA_UP): {
	if(state.row_ticks == 0) //
	  break;
	chn->actual_finetune+=param*6;
	if(chn->actual_finetune>N_FINETUNE_STEPS) {
	  chn->note++;
	  chn->actual_finetune-=N_FINETUNE_STEPS;
	}
	if (chn->smp == 0) {
	  song->setoops(OOPS_EFF_WO_SMP,state.pattern,state.row,channel);
	  break;
	}
	if (chn->actual_finetune>0) 
	  chn->smp->bendNote(chn->note, chn->actual_finetune, chn);
	else 
	  chn->smp->bendNote(chn->note-1, 
			     chn->actual_finetune+N_FINETUNE_STEPS, chn);
	
	song->setoops(OOPS_XP_PORTA_UP|0x8000,state.pattern,state.row,channel);
	break;
      }
      case(EFFECT_PORTA_DN): {
	if(state.row_ticks == 0) //
	  break;
	chn->actual_finetune-=param*6;
	if(chn->actual_finetune<-N_FINETUNE_STEPS) {
	  chn->note--;
	  chn->actual_finetune+=N_FINETUNE_STEPS;
	}
	
	if (chn->smp == 0) {
	  song->setoops(OOPS_EFF_WO_SMP,state.pattern,state.row,channel);
	  break;
	}
	if (chn->actual_finetune>0)
	  chn->smp->bendNote(chn->note, chn->actual_finetune, chn);
	else 
	  chn->smp->bendNote(chn->note-1, 
			     chn->actual_finetune+N_FINETUNE_STEPS, chn);
	song->setoops(OOPS_XP_PORTA_DN|0x8000,state.pattern,state.row,channel);
	break;
      }
      case(EFFECT_PORTA_NOTE): {
	if(state.row_ticks == 0) //
	  break;
	chn->actual_finetune+=chn->slidetune_speed*6;
	// update note, halt if needed.
	while (chn->slidetune_speed<0 && chn->actual_finetune<-N_FINETUNE_STEPS) {
	  // sliding down and crossed a halftone.
	  chn->note--;
	  if (chn->note==chn->target_note) 
	    chn->actual_finetune=chn->slidetune_speed=0; // stop if done.
	  else
	    chn->actual_finetune+=N_FINETUNE_STEPS;
	}
	while (chn->slidetune_speed>0 && chn->actual_finetune>N_FINETUNE_STEPS) {
	  // sliding up and crossed a halftone.
	  chn->note++;
	  if (chn->note==chn->target_note) 
	    chn->actual_finetune=chn->slidetune_speed=0;
	  else
	    chn->actual_finetune-=N_FINETUNE_STEPS;
	}

	// now check the actual finetune and update playout freq.
	if (chn->actual_finetune>0)
	  chn->smp->bendNote(chn->note, chn->actual_finetune, chn);
	else 
	  chn->smp->bendNote(chn->note-1, 
			     chn->actual_finetune+N_FINETUNE_STEPS, chn);

	break;
      }
      case(EFFECT_VIBRATO):{
	if(state.row_ticks == 0) //
	  break;
	
	u8 pos = (chn->vibpos>>2)&31;
	// sine vibratos only atm. XM also has ramps, square and rnd waves
	unsigned vib = VibratoTable[pos];
	
	// vib is 0..255. 0 is "play the note as it is", 255 is top deviation
	// beyond the "normal" note. Mikmod allows vibrato to happen above the
	// current note as well as below that note (when 'position' is negative)
	// i'm unsure at how a negative vibspeed is encoded in the pattern, anyway
	
	// the depth ranges from 0 to 15.

	// mikmod later does vib = (wave * depth / 128)*4
	// remind that mikmod does period += speed*4, meaning that "period" is
	// expressed in 4-XM-portamento-units here. A portamento unit is 1/255th
	// of octave.
	// the max. amplitude for the vibrato is 255*15/128 ~= 30 portamento units
	// (while a half-tone is ~21.3 portamento units)
	vib=((vib*chn->vibdepth)>>7)*6;

	chn->smp->bendNote(chn->note + (vib/N_FINETUNE_STEPS),
			   vib%N_FINETUNE_STEPS,
			   chn);

	chn->vibpos+=chn->vibspeed;
	break;
      }
      case(EFFECT_VOLUME_SLIDE): 
	if(state.row_ticks == 0) //
	  break;
	
	s8 slidespeed;
	
	if(param == 0)
	  slidespeed = effstate.channel_last_slidespeed[channel];
	else if( (param & 0x0F) == 0 )
	  slidespeed = (param >> 4) * 2;
	else
	  slidespeed = -(param & 0x0F) * 2;
	
	effstate.channel_last_slidespeed[channel] = slidespeed;
	
	s16 targetvolume = state.channel_volume[channel] + slidespeed;
	if(targetvolume > MAX_VOLUME)
	  targetvolume = MAX_VOLUME;
	else if(targetvolume < 0)
	  targetvolume = 0;
	
	effstate.channel_setvol_requested[channel] = true;
	state.channel_fade_target_volume[channel] = targetvolume;
	
	break;
      
	
      }
    }
  }
}

//! we're about to read a new row. Terminate the running effects (if needed).
/*! When this function is called, state.pattern and state.row have already
 *  been updated to reflect the new playing state, but we haven't called playRow
 *  so the buffered Cell is still valid.
 */
void Player::finishEffects(void)
{
  struct ChannelState *chn=chan;
  for(u8 i=0; i < MAX_CHANNELS; i++,chn++) {
    if (i >= song->n_channels && chan[i].current_cell==0) continue;
	
    u8 effect = chn->cell.effect;
    u8 new_effect = song->patterns[state.pattern][i][state.row].effect;
    //u8 param = state.channel_effect_param[channel];
    // u8 instidx = state.channel_instrument[channel];
    // Instrument *inst = chan[channel].inst;
    //      song->instruments[instidx];
    
    if( (effect != NO_EFFECT) && (new_effect != effect) )
      {
	switch(effect)
	  {
	  case(EFFECT_ARPEGGIO):
	    {
	      if (chn->smp == 0)
		continue;
	      
	      // Reset note
	      chn->smp->bendNote(chn->note, 0, chn);
	      
	      break;
	    }
	  }
      }
  }
}

void Player::initState(void)
{
  state.row = 0;
  state.pattern = 0;
  state.potpos = 0;
  state.playing = false;
  state.waitrow = false;
  state.patternloop = false;
  state.songloop = true;
  my_memset(state.channel_active, 0, sizeof(state.channel_active));
  my_memset(state.channel_ms_left, 0, sizeof(state.channel_ms_left));
  my_memset(state.channel_note, EMPTY_NOTE, sizeof(state.channel_note));

  {
    //    ChannelState inistate(0);
    for (int i=0;i<MAX_CHANNELS;i++){
      // inistate.no=i;
      chan[i]=ChannelState(i);
    }
  }
  my_memset(state.channel_fade_active, 0, sizeof(state.channel_fade_active));
  my_memset(state.channel_fade_ms, 0, sizeof(state.channel_fade_ms));
  my_memset(state.channel_fade_target_volume, 0, sizeof(state.channel_fade_target_volume));
  my_memset(state.channel_volume, 0, sizeof(state.channel_volume));
  my_memset(state.channel_env_vol, 63, sizeof(state.channel_env_vol));
  my_memset(state.channel_fade_vol, 127, sizeof(state.channel_fade_vol));
  state.playing_single_sample = false;
  state.single_sample_ms_remaining = 0;
  state.single_sample_channel = 0;
}

void Player::initEffState(void)
{
  effstate.pattern_loop_begin = 0;
  effstate.pattern_loop_count = 0;
  effstate.pattern_loop_jump_now = false;
  my_memset(effstate.channel_setvol_requested, false, sizeof(effstate.channel_setvol_requested));
  my_memset(effstate.channel_last_slidespeed, 0, sizeof(effstate.channel_last_slidespeed));
  effstate.pattern_break_requested = false;
  effstate.pattern_break_row = 0;
  effstate.pattern_jump_requested=false;
}

void Player::handleFade(u32 passed_time)
{
  // Find channels that need to be faded
  for(u8 channel=0; channel<MAX_CHANNELS; ++channel)
    {
      if(state.channel_fade_active[channel] == 1)
	{
	  // Decrement ms until fade is complete
	  if(state.channel_fade_ms[channel] > passed_time)
	    state.channel_fade_ms[channel] -= passed_time;
	  else
	    state.channel_fade_ms[channel] = 0;
	  
	  // Calculate volume from initial volume, target volume and remaining fade time
	  // Can be done way quicker using bresenham
	  float fslope = (float)(state.channel_volume[channel] - state.channel_fade_target_volume[channel])
	    / (float)FADE_OUT_MS;
	  
	  float fvolume = (float)state.channel_fade_target_volume[channel]
	    + fslope * (float)(state.channel_fade_ms[channel]);
	  
	  u8 volume = (u8)fvolume;
	  state.channel_fade_vol[channel] = volume;
	  
	  
	  // If we reached 0 ms, disable the fader
	  if(state.channel_fade_ms[channel] == 0)
	    {
	      state.channel_fade_active[channel] = 0;     
	      state.channel_volume[channel] = state.channel_fade_target_volume[channel];
	      state.channel_fade_vol[channel] = state.channel_fade_target_volume[channel];
	    }
	}
    }
}

// Calculate next row and pot position
// Returns true if the song is finished, false else
bool Player::calcNextPos(u16 *nextrow, u8 *nextpotpos)
{
  if (effstate.pattern_jump_requested) {
    *nextrow = 0;
    *nextpotpos = effstate.pattern_jump_order;
    return false;
  }
	
  if(effstate.pattern_loop_jump_now == true)
    {
      *nextrow = effstate.pattern_loop_begin;
      *nextpotpos = state.potpos;
      
      return false;
    }
  
  if(effstate.pattern_break_requested == true) {
      *nextrow = effstate.pattern_break_row;
      
      if(state.potpos < song->getPotLength() - 1)
	*nextpotpos = state.potpos + 1;
      else
	*nextpotpos = song->getRestartPosition();
      
      return false;
  }
	
  if(state.row + 1 >= song->patternlengths[state.pattern])
    {
      if(state.patternloop == false) // Don't jump when looping is enabled
	{
	  if(state.potpos < song->getPotLength() - 1)
	    *nextpotpos = state.potpos + 1;
	  else if(state.songloop == true)
	    *nextpotpos = song->getRestartPosition();
	  else
	    return true;
	} else {
	*nextpotpos = state.potpos;
      }
      *nextrow = 0;
    }
  else
    {
      *nextrow = state.row + 1;
      *nextpotpos = state.potpos;
    }
  
  return false;
}
