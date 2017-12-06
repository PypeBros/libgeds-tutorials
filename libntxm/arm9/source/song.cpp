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

#include "ntxm/song.h"


#include "tools.h"

#include <stdlib.h>
#include <stdio.h>

#ifdef ARM7
#include "demokit.h"
#endif

#include "command.h"

/*
A word on pattern memory management:
patterns is a 3d-array. The dimensions are
pattern - channel - row
The first dimension is completely allocated,
the second and third dimensions are only
allocated as far as needed.
*/

/* ===================== PUBLIC ===================== */

// Everything that changes the song is only possible on arm9.
// This saves memory on arm7 (there's only 96k) and is safer.

void Song::setoops(int code, int pat, int row, int chan) {

  if (oops[0]!=0 && (oops[0]&0xf000)<(code&0xf000)) return; // we gave out a more important hint
  if (oops[1]!=-1) return; // there is already an unacknowledged error.
  oops[0]=code; oops[1]=pat; oops[2]=(row<<16)|chan;
}

#ifdef ARM9

int Song::getoops(int* pat, int *row, int* chan) {
  if (oops[0]==0 || oops[1]==-1) return 0; // nothing to report.
  if (pat) *pat=oops[1];
  if (row) *row=oops[2]>>16;
  if (chan)*chan=oops[2]&0xff;
  oops[1]=-1; oops[2]=-1;
  int code=oops[0]; oops[0]=0;
  return code;
}

Song::Song(u8 _speed, u8 _bpm, u8 _channels)
  :NtMagic("SONG"), speed(_speed), bpm(_bpm), n_channels(_channels), restart_position(0), 
   patternlengths(), internal_patternlengths(), pattern_order_table(),
   instruments(), name(), n_patterns(0), potsize(0), patterns()
{
  oops[0]=0; oops[1]=-1; oops[2]=-1; oops[3]=-1;
  // Init arrays
  //patternlengths = (u16*)_malloc(sizeof(u16)*MAX_PATTERNS);
  //internal_patternlengths = (u16*)_malloc(sizeof(u16)*MAX_PATTERNS);
  //pattern_order_table = (u8*)_malloc(sizeof(u8)*MAX_POT_LENGTH);
  //instruments = (Instrument**)_malloc(sizeof(Instrument*)*MAX_INSTRUMENTS);
  //name = (char*)_malloc(MAX_SONG_NAME_LENGTH+1);
  // my_memset(name, 0, MAX_SONG_NAME_LENGTH+1);
  my_strncpy(name, "unnamed", MAX_SONG_NAME_LENGTH);
  
  for(u16 i=0; i<MAX_PATTERNS; ++i) {
    patternlengths[i] = 0;
    internal_patternlengths[i] = 0;
  }
	
	for(u16 i=0; i<MAX_POT_LENGTH; ++i) {
		pattern_order_table[i] = 0;
	}
	
	for(u16 i=0; i<MAX_INSTRUMENTS; ++i) {
		instruments[i] = NULL;
	}
	
	n_patterns = 0;
	potsize = 0;

	my_memset(channels_muted, false, MAX_CHANNELS * sizeof(bool));
	
	// Init pattern array
//patterns = (Cell***)_malloc(sizeof(Cell**)*MAX_PATTERNS);

	// Create first pattern
	addPattern();

	// Init pattern order table
	potIns(0, 0);
}

Song::~Song()
{
  printf("deleting song...\n");
	killInstruments();
	
	killPatterns();

	// Delete arrays
	// _free(patternlengths);
	// _free(internal_patternlengths);
	// _free(pattern_order_table);
	// _free(name);
}

#endif

Cell **Song::getPattern(u8 idx)
{
	if(idx<n_patterns) {
		return patterns[idx];
	} else {
		return 0;
	}
}


u8 Song::getChannels(void) {
	return n_channels;
}

u16 Song::getPatternLength(u8 idx)
{
	if(idx<n_patterns) {
		return patternlengths[idx];
	} else {
		return 0;
	}
}

// How many milliseconds per row
u32 Song::getMsPerRow(void) {
	return speed*5*1000/2/bpm; // Formula from Fasttracker II
}

// How many milliseconds per tick (1 tick = time for 1 row / speed)
u32 Song::getMsPerTick(void) {
	return 5*1000/2/bpm; // Formula from Fasttracker II
}

Instrument *Song::getInstrument(u8 instidx) {
	return instruments[instidx];
}

u8 Song::getInstruments(void)
{
	// Return highest instrument index+1
	u8 n_inst = 0;
	for(u8 i=0; i<MAX_INSTRUMENTS; ++i) {
		if( instruments[i] != 0 ) {
			n_inst = i+1;
		}
	}
	return n_inst;
}

#ifdef ARM9

void Song::setInstrument(u8 idx, Instrument *instrument) {
	instruments[idx] = instrument;
}

// POT functions
void Song::potAdd(u8 ptn)
{
	pattern_order_table[potsize] = ptn;
	potsize++;
}

void Song::potDel(u8 element)
{
	for(u16 i=element; i<potsize; ++i) {
		pattern_order_table[i] = pattern_order_table[i+1];
	}
	if(potsize > 1) {
		potsize--;
	}	
}

void Song::potIns(u8 idx, u8 pattern)
{
	if(potsize < MAX_POT_LENGTH) {
		for(u8 i=255;i>idx;--i) {
			pattern_order_table[i] = pattern_order_table[i-1];
		}
		pattern_order_table[idx] = pattern;
		potsize++;
	}
}

#endif

u8 Song::getPotLength(void) {
	return potsize;
}

u8 Song::getPotEntry(u8 idx) {
	return pattern_order_table[idx];
}

#ifdef ARM9

void Song::setPotEntry(u8 idx, u8 value) {
	pattern_order_table[idx] = value;
}
void Song::addPattern(u16 length)
{
	patternlengths[n_patterns] = length;
	internal_patternlengths[n_patterns] = length;

	n_patterns++;
	
	patterns[n_patterns-1] = (Cell**)_malloc(sizeof(Cell*)*n_channels);

	u8 i,j;
	for(i=0;i<n_channels;++i) {
		patterns[n_patterns-1][i] = (Cell*)_malloc(sizeof(Cell)*patternlengths[n_patterns-1]);
		Cell *cell;
		for(j=0;j<patternlengths[n_patterns-1];++j) {
			cell = &patterns[n_patterns-1][i][j];
			clearCell(cell);
		}
	}
	
}

void Song::channelAdd(void) {
	
	if(n_channels==MAX_CHANNELS) return;
	
	// Go through all patterns and add a channel
	for(u8 pattern=0;pattern<n_patterns;++pattern) {
		patterns[pattern] = (Cell**)_realloc(patterns[pattern], sizeof(Cell*)*(n_channels+1));
		patterns[pattern][n_channels] = (Cell*)_malloc(sizeof(Cell)*internal_patternlengths[pattern]);
		
		// Clear
		Cell *cell;
		for(u8 j=0;j<internal_patternlengths[pattern];++j) {
			cell = &patterns[pattern][n_channels][j];
			clearCell(cell);
		}
	}
	
	n_channels++;
	
}

void Song::channelDel(void) {
	
	if(n_channels==1) return;
	
	// Go through all patterns and delete the last channel
	for(u8 pattern=0;pattern<n_patterns;++pattern) {
		_free(patterns[pattern][n_channels-1]);
		patterns[pattern] = (Cell**)_realloc(patterns[pattern], sizeof(Cell*)*(n_channels-1));
	}
	
	n_channels--;
	
}
#endif

u8 Song::getNumPatterns(void) {
	return n_patterns;
}

#ifdef ARM9
void Song::resizePattern(u8 ptn, u16 newlength)
{
	// If the pattern is shortened or if the pattern is enlarged,
	// but stays below or equal to the internal length
	if( ( newlength <= patternlengths[ptn] ) ||
	    ( (newlength > patternlengths[ptn]) && (newlength <= internal_patternlengths[ptn]) ) )
	{
		patternlengths[ptn] = newlength; // Only set the length value
	
	} else { // If the pattern is enlarged beyond the internal length
	
		// Go through all channels of this pattern and resize them
		for(u8 channel=0; channel<n_channels; ++channel) {
			patterns[ptn][channel] = (Cell*)_realloc(patterns[ptn][channel], sizeof(Cell)*newlength);
			
			// The new cells must be cleared
			Cell *cell;
			for(u16 i=internal_patternlengths[ptn]; i<newlength;++i) {
				cell = &patterns[ptn][channel][i];
				clearCell(cell);
			}
		}
		
		patternlengths[ptn] = newlength;
		internal_patternlengths[ptn] = newlength;
	}
}

// The most important function
void Song::setName(const char *_name) {
	my_strncpy(name, _name, MAX_SONG_NAME_LENGTH);
}

#endif

const char *Song::getName(void) {
	return name;
}

#ifdef ARM9

void Song::setRestartPosition(u8 _restart_position) {
	restart_position = _restart_position;
}

#endif

u8 Song::getRestartPosition(void) {
	return restart_position;
}

u8 Song::getTempo(void) {
  return speed;
}

u8 Song::getBPM(void) {
	return bpm;
}


void Song::setTempo(u8 _tempo) {
	speed = _tempo;
}

void Song::setBpm(u8 _bpm) {
	bpm = _bpm;
}
#ifdef ARM9
// Zapping
void Song::zapPatterns(void) {
	
	while(potsize > 1) {
		potDel(potsize-1);
	}
	setPotEntry(0, 0);
	
	killPatterns();
	n_channels = DEFAULT_CHANNELS;
	n_patterns = 0;
	
	// patterns = (Cell***)_malloc(sizeof(Cell**)*MAX_PATTERNS);
	
	addPattern();
	
	restart_position = 0;
}

void Song::zapInstruments(void)
{
	killInstruments();
	
	//	instruments = (Instrument**)_malloc(sizeof(Instrument*)*MAX_INSTRUMENTS);
	for(u16 i=0; i<MAX_INSTRUMENTS; ++i) {
		instruments[i] = NULL;
	}
	
}

void Song::clearCell(Cell *cell)
{
	cell->note = EMPTY_NOTE;
	cell->volume = NO_VOLUME;
	cell->instrument = NO_INSTRUMENT;
	cell->effect = NO_EFFECT;
	cell->effect_param = NO_EFFECT_PARAM;
	cell->effect2 = NO_EFFECT;
	cell->effect2_param = NO_EFFECT_PARAM;
}

void Song::setChannelMute(u8 chn, bool muted, bool force)
{
	if(chn >= n_channels && !force)
		return;
	
	channels_muted[chn] = muted;
}

#endif

bool Song::channelMuted(u8 chn)
{
	if(chn >= n_channels)
		return false;
	
	return channels_muted[chn];
}

/* ===================== PRIVATE ===================== */

#ifdef ARM9

void Song::killPatterns(void) {
	
	for(u8 ptn=0; ptn<n_patterns; ++ptn) {
		
		for(u8 chn=0; chn<n_channels; ++chn) {
			
			_free(patterns[ptn][chn]);
		}
		
		_free(patterns[ptn]);
	}
	// _free(patterns);
}

void Song::killInstruments(void) {
	
	for(u8 i=0;i<MAX_INSTRUMENTS;++i) {
		if(instruments[i] != NULL) {
			delete instruments[i];
			instruments[i] = NULL;
		}
	}
	// _free(instruments);
}

#endif
