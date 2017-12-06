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

#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "song.h"
#include <stddef.h>
#include <new>

#define FADE_OUT_MS	10 // Milliseconds that a click-preventing fadeout takes

typedef unsigned int bitmask;

typedef struct
{
	u16 row;							// Current row
	u8 pattern;							// Current pattern
	u8 potpos;							// Current position in pattern order table
	bool playing;						// D'uh!
	bool juststarted;						// If we just started playing
	u32 tick_ms;						// ms spent in the current tick
	u8 row_ticks;						// Ticks that passed in the current row
	u8 channel_active[MAX_CHANNELS];			// 0 for inactive, 1 for active
	u16 channel_ms_left[MAX_CHANNELS];			// how many milliseconds still to play?
	bool channel_loop[MAX_CHANNELS]; 			// Is the sample that is played looped?
	u8 channel_fade_active[MAX_CHANNELS];		// Is fadeout for channel i active?
	u8 channel_fade_ms[MAX_CHANNELS];			// How long (ms) till channel i is faded out?
	u8 channel_fade_target_volume[MAX_CHANNELS];	// Target volume after fading
	u8 channel_volume[MAX_CHANNELS];			// Current channel volume
	u8 channel_note[MAX_CHANNELS];			// Current note playing
	u8 channel_env_vol[MAX_CHANNELS];			// Current envelope height (0..63)
	u8 channel_fade_vol[MAX_CHANNELS];			// Current fading volume (0..127)
	bool waitrow;						// Wait until the end of the last tick before muting instruments
	bool patternloop;						// Loop the current pattern
	bool songloop;			// Loop the entire song
	
	bool playing_single_sample;
	u32 single_sample_ms_remaining;
	u8 single_sample_channel;
} PlayerState;
#include "channel.h"

typedef struct {
  u16 pattern_loop_begin;
  u8 pattern_loop_count;
  bool pattern_loop_jump_now;
  bool channel_setvol_requested[MAX_CHANNELS];
  s16 channel_last_slidespeed[MAX_CHANNELS]; // ! for volume slides.
//  s16 actual_finetune[MAX_CHANNELS];
  bool pattern_break_requested;
  u8 pattern_break_row;
  bool pattern_jump_requested;
  u8 pattern_jump_order;
} EffectState;

class Player : NtMagic {
 public:
  
  // Constructor. The first arument is a function pointer to a function that calls the
  // playTimerHandler() funtion of the player. This is a complicated solution, but
  // the timer callback must be a static function.
  Player(void (*_externalTimerHandler)(void)=0);
  
  // override new and delete to avoid linking cruft. (by WinterMute)
  static void* operator new (size_t size);
  static void operator delete (void *p);
  
  //
  // Play Control
  //
  
  void setSong(Song *_song);
  
  // Set the current pattern to looping
  void setPatternLoop(bool loopstate);
  
  // Plays the song till the end starting at the given pattern order table position and row
  void play(u8 potpos, u16 row, bool loop);
  
  // Plays on the specified pattern
  void playPtn(u8 ptn);
  
  void stop(void);
  
  // Play the note with the given settings
  void playNote(u8 note, u8 volume, struct ChannelState* channel, u8 instidx);
  
  void playSfx(u8 pot, u8 row, u8 channel, u8 nchannels);

  // Play the given sample (and send a notification when done)
  void playSample(Sample *sample, u8 note, u8 volume, u8 channel);
  
  // Suited for commands.
  void playInstrument(u32 instr, u32 note, u32 volume, u32 channel);

  // Stop playback on a channel
  void stopChannel(u8 channel);
  
  //
  // Callbacks
  //
  
  void registerRowCallback(void (*onRow_)(u16));
  void registerPatternChangeCallback(void (*onPatternChange_)(u8));
  void registerSampleFinishCallback(void (*onSampleFinish_)());
  
  //
  // Misc
  //
  
  void playTimerHandler(void);
  void stopSampleFadeoutTimerHandler(void);
  
 private:
  
  void startPlayTimer(void);
  void playRow(void);
  void handleEffects(void); // Row Effect handler
  void handleTickEffects(void); // Tick Effect handler
  void finishEffects(void); // Clean up after the effects
  
  void initState(void);
  void initEffState(void);
  
  void handleFade(u32 passed_time);
  
  // Calculate next row and pot position
  // Returns true is the song is finished, false else
  bool calcNextPos(u16 *nextrow, u8 *nextpotpos);
  
  Song *song;
  static PlayerState state;
  static EffectState effstate;
  static struct ChannelState chan[MAX_CHANNELS];  
  void (*externalTimerHandler)(void);
  void (*onRow)(u16);
  void (*onPatternChange)(u8);
  void (*onSampleFinish)();
  
  u32 lastms; // For timer
};

#endif
