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

#include "ntxm/sample.h"

#include "command.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ntxm/channel.h"
#ifdef ARM9
#include "tools.h"
#endif

#define MAX(x,y)					((x)>(y)?(x):(y))
#define LOOKUP_FREQ(note,finetune)		(linear_freq_table_lookup(MAX(0,N_FINETUNE_STEPS*(note)+(finetune))))

// This is defined in audio.h for arm7 but not for arm9
#if !defined(SOUND_FORMAT_ADPCM)
#define SOUND_FORMAT_ADPCM	(2<<29)
#define SOUND_16BIT 		(1<<29)
#define SOUND_8BIT 		(0)
#endif



/* ===================== PUBLIC ===================== */

#ifdef ARM9

Sample::Sample(void *_sound_data, u32 _n_samples, u16 _sampling_frequency, bool _is_16_bit,
	       u8 _loop, u8 _volume)
  : NtMagic("NSMP"), sound_data(), original_data(0), pingpong_data(0), 
   n_samples(_n_samples), original_n_samples(0),
   is_16_bit(_is_16_bit), loop(_loop),
    rel_note(0), finetune(0), sizes(), loop_start(0), loop_length(0), volume(_volume), 
   sound_format(0)
{
  // sound_data = (void**)_calloc(20*sizeof(void*), 1);
  sound_data[0] = _sound_data;
  
  memset(name, 0, SAMPLE_NAME_LENGTH);
  
  calcSize();
  setFormat();
  calcRelnoteAndFinetune(_sampling_frequency);
  
  if (!make_subsampled_versions())
    iprintf("not enough memory for sub-samples (%i)!",n_samples);
  
  setLoopStartAndLength(0, _n_samples);
}

Sample::~Sample()
{
  // if (sound_data) {
    iprintf("(~smp %p)",sound_data[0]);
    delete_subsampled_versions();
    _free(sound_data[0]);
    if(pingpong_data != 0 && pingpong_data != sound_data[0]) {
      iprintf("deleting pingpong data");
      _free(pingpong_data);
    }
    if (original_data && original_data != sound_data[0]) {
      iprintf("deleting looped sample");
      _free(original_data);
    }
    // _free(sound_data);
    //} else iprintf("sample has no data!\n");
}

#endif

inline u32 linear_freq_table_lookup(u32 note)
{
	/*
	// readable version
	if(note<=LINEAR_FREQ_TABLE_MAX_NOTE*N_FINETUNE_STEPS) {
		if(note>=LINEAR_FREQ_TABLE_MIN_NOTE*N_FINETUNE_STEPS) {
			return linear_freq_table[note-LINEAR_FREQ_TABLE_MIN_NOTE*N_FINETUNE_STEPS];
		} else {
			u32 octaveoffset = ((LINEAR_FREQ_TABLE_MIN_NOTE*N_FINETUNE_STEPS-1)-note) / (12*N_FINETUNE_STEPS) + 1;
			u32 relnote = note % (12*N_FINETUNE_STEPS);
			#ifdef ARM7
			//CommandDbgOut("minoct:%u noteoct:%u\n",
			//	      (LINEAR_FREQ_TABLE_MIN_NOTE*N_FINETUNE_STEPS-1)/(12*N_FINETUNE_STEPS),
			//	      note/(12*N_FINETUNE_STEPS)
			//	     );
			#endif
			#ifdef ARM9
			iprintf("%u %u\n",octaveoffset,relnote);
			#endif
			return linear_freq_table[relnote] >> octaveoffset;
		}
	}
	return 0;
	*/
	
	// fast version
	if(note<=LINEAR_FREQ_TABLE_MAX_NOTE*N_FINETUNE_STEPS)
	{
		if(note>=LINEAR_FREQ_TABLE_MIN_NOTE*N_FINETUNE_STEPS) {
			return linear_freq_table[note-LINEAR_FREQ_TABLE_MIN_NOTE*N_FINETUNE_STEPS];
		} else {
			return linear_freq_table[note % (12*N_FINETUNE_STEPS)] >>
				(((LINEAR_FREQ_TABLE_MIN_NOTE*N_FINETUNE_STEPS-1)-note) / (12*N_FINETUNE_STEPS)  + 1);
		}
	}
	return 0;
}

#if defined(ARM7)

// used for offset effects.
void Sample::advance(/*u8 note, */long offset, struct ChannelState* chn) {
/*  u8 absolute_note = note + 48;
    u8 octave = (absolute_note+rel_note) / 12;
    u8 srcsmp;
    
    if(octave <= 12)
    srcsmp = 0;
    else 
    srcsmp = octave-12;
*/
  
  SCHANNEL_SOURCE(chn->no) = (uint32)sound_data[chn->srcsmp_cache] + offset;
}

// volume_ ranges from 0-127. The value 255 means "no volume", i.e. the sample's own volume shall be used.
void Sample::play(u8 note, u8 volume_ , struct ChannelState* chn)
{
  u8 channel=chn->no;
  if(channel>15) return; // DS has only 16 channels!
	
	/*
	if(note+rel_note > N_LINEAR_FREQ_TABLE_NOTES) {
		CommandDbgOut("Freq out of range!\n");
		return;
	}
	*/
	
	u32 loop_bit;
	if( ( ( loop == FORWARD_LOOP ) || (loop == PING_PONG_LOOP) ) && (loop_length > 0) )
		loop_bit = SOUND_REPEAT;
	else
		loop_bit = SOUND_ONE_SHOT;
	
	// Add 48 to the note, because otherwise absolute_note can get negative.
	// (The minimum value of relative note is -48)
	u8 absolute_note = note + 48;
	
	// Choose the subsampled version. The first 12 octaves will be fine,
	// if the note is higher, choose a subsampled version.
	// Octave 12 is more of a good guess, so there could be better, more
	// reasonable values.
	u8 octave = (absolute_note+rel_note) / 12;
	u8 srcsmp, realnote;
	
	
	if(octave <= 12) {
	  srcsmp = 0;
	  realnote = (absolute_note+rel_note);
	} else {
	  srcsmp = octave-12;
	  realnote = (absolute_note+rel_note) % 12 + 12*12;
	}
	chn->srcsmp_cache=srcsmp;
	// If a volume is given, it overrides the sample's own volume
	u8 smpvolume;
	if(volume_ == NO_VOLUME)
	  smpvolume = volume / 2; // Smp volume is 0..255
	else
	  smpvolume = volume_; // Channel volume is 0..127
	
	SCHANNEL_CR(channel) = 0;
	SCHANNEL_TIMER(channel) = SOUND_FREQ((int)LOOKUP_FREQ(realnote,finetune));
	SCHANNEL_SOURCE(channel) = (uint32)sound_data[srcsmp];
	
	if( loop == NO_LOOP )
	{
		SCHANNEL_REPEAT_POINT(channel) = 0;
		SCHANNEL_LENGTH(channel) = sizes[srcsmp] >> 2;
	}
	else if( loop == FORWARD_LOOP )
	{
		SCHANNEL_REPEAT_POINT(channel) = loop_starts[srcsmp] >> 2;
		SCHANNEL_LENGTH(channel) = loop_lengths[srcsmp] >> 2;
	}
	else if( loop == PING_PONG_LOOP )
	{
		SCHANNEL_REPEAT_POINT(channel) = loop_starts[srcsmp] >> 2;
		SCHANNEL_LENGTH(channel) = loop_lengths[srcsmp] >> 1;
	}
	
	SCHANNEL_CR(channel) =
		SCHANNEL_ENABLE |
		loop_bit |
		sound_format |
		SOUND_PAN(64) |
		SOUND_VOL(smpvolume);
}

void Sample::bendNote(u8 note, u8 finetune, ChannelState *channel)
{
	// Add 48 to the note, because otherwise absolute_note can get negative.
	// (The minimum value of relative note is -48)
	u8 absolute_note = note + 48;
	u8 /*srcsmp,*/ realnote;
/*	
	u8 base_absolute_note = basenote + 48;
	u8 base_octave = (base_absolute_note+rel_note) / 12;
	
	if(base_octave <= 12) {
		srcsmp = 0;
		realnote = (absolute_note+rel_note);
	} else {
		srcsmp = base_octave-12;
		realnote = (absolute_note+rel_note) - 12 * srcsmp;
	}
*/
	realnote = absolute_note+rel_note - 12 * channel->srcsmp_cache;
	/** LOOKUP_FREQ(note,finetune)              
	 *      (linear_freq_table_lookup(MAX(0,N_FINETUNE_STEPS*(note)+(finetune))))
	 */
	SCHANNEL_TIMER(channel->no) = SOUND_FREQ((int)LOOKUP_FREQ(realnote,finetune));
}

#endif

u32 Sample::calcPlayLength(u8 note)
{
	u32 samples_per_second = LOOKUP_FREQ(48+note+rel_note,finetune);
	return n_samples * 1000 / samples_per_second;
}

#ifdef ARM9

void Sample::setRelNote(s8 _rel_note) {
	rel_note = _rel_note;
}

void Sample::setFinetune(s8 _finetune) {
	finetune = _finetune;
}

#endif

u8 Sample::getRelNote(void) {
	return rel_note;
}

s8 Sample::getFinetune(void) {
	return finetune;
}

u32 Sample::getSize(void) {
	return sizes[0];
}

u32 Sample::getNSamples(void)
{
	if(pingpong_data != 0)
		return original_n_samples;
	else
		return n_samples;
}

void *Sample::getData(void)
{
	// sound_data[0] is modified for the loop, but original_data points to the unmodified sound data
	if(pingpong_data != 0)
		return original_data;
	else
		return sound_data[0];
}

u8 Sample::getLoop(void) {
	return loop;
}

#ifdef ARM9

bool Sample::setLoop(u8 loop_) // Set loop type. Can fail due to memory constraints
{
	if(loop_ == loop)
	  return true;  // we're not modifying anything :P
	
	if(/*loop == PING_PONG_LOOP ||*/
	   pingpong_data
	   ) // Switching from ping-pong to sth else
	{
	  removePingPongLoop();
	}
	
	loop = loop_;
	if(loop_ == FORWARD_LOOP && 
	   (loop_length&3)) {
	  int octaves_up = (rel_note - BASE_NOTE)/12;
	  iprintf("%s: odd loop, %i / %i",name,loop_length,1<<octaves_up);
	  if(octaves_up>0 && 
	     loop_length / (1<<octaves_up) < 512)
	    return setupOddLoop();
	  if (octaves_up<=0 && loop_length < 512)
	    return setupOddLoop();
	}
	if(loop_ == PING_PONG_LOOP)
	{
	  // Check if enough memory is available
	  u8 *testmem = (u8*)_malloc(2*loop_length + sizes[0]);
	  if(testmem == 0)
	    return false;
	  _free(testmem);
	  
	  setupPingPongLoop();
	}
	
	return true;
}

#endif

bool Sample::is16bit(void) {
	return is_16_bit;
}

u32 Sample::getLoopStart(void)
{
	if(is_16_bit)
		return loop_start / 2;
	else
		return loop_start;
}

#ifdef ARM9

void Sample::setLoopStart(u32 _loop_start)
{
	u32 loop_length_in_samples;
	if(is_16_bit)
		loop_length_in_samples = loop_length / 2;
	else
		loop_length_in_samples = loop_length;
	
	_loop_start = clamp(_loop_start, 0, n_samples-1);
	loop_length_in_samples = clamp(loop_length_in_samples, 0, n_samples-1 - _loop_start);
	
	setLoopStartAndLength(_loop_start, loop_length_in_samples);
}

void Sample::setLoopLength(u32 _loop_length)
{
	u32 loop_start_in_samples;
	if(is_16_bit)
		loop_start_in_samples = loop_start / 2;
	else
		loop_start_in_samples = loop_start;
	
	loop_start_in_samples = clamp(loop_start_in_samples, 0, n_samples-1);
	_loop_length = clamp(_loop_length, 0, n_samples-1 - loop_start_in_samples);
	
	setLoopStartAndLength(loop_start_in_samples, _loop_length);
}

void Sample::setLoopStartAndLength(u32 _loop_start, u32 _loop_length)
{
	if(is_16_bit)
	{
		loop_length = _loop_length * 2;
		loop_lengths[0] = _loop_length * 2;
		loop_start = _loop_start * 2;
		loop_starts[0] = _loop_start * 2;
	}
	else
	{
		loop_length = _loop_length;
		loop_lengths[0] = _loop_length;
		loop_start = _loop_start;
		loop_starts[0] = _loop_start;
	}
	
	for(u8 i=1;i<20;++i) {
		loop_lengths[i] = loop_lengths[i-1] / 2;
		loop_starts[i] = loop_starts[i-1] / 2;
	}
	
	if( loop == PING_PONG_LOOP )
		updatePingPongLoop();
}

#endif

u32 Sample::getLoopLength(void)
{
	if(is_16_bit)
		return loop_length / 2;
	else
		return loop_length;
}

void Sample::setVolume(u8 vol) {
	volume = vol;
}

u8 Sample::getVolume(void) {
	return volume;
}

void Sample::setName(const char *name_)
{
	strncpy(name, name_, SAMPLE_NAME_LENGTH-1);
}

const char *Sample::getName(void)
{
	return name;
}

#ifdef ARM9

// Deletes the part between start sample and end sample
void Sample::delPart(u32 startsample, u32 endsample)
{
	if(endsample >= n_samples)
		endsample = n_samples-1;
	
	// Special case: everything is deleted
	if((startsample==0)&&(endsample==n_samples)) {
		_free(sound_data[0]);
		n_samples = 0;
		calcSize();
		loop_start = loop_length = 0;
		for(u8 octave = 1;octave<20;++octave) {
			sound_data[octave] = _realloc(sound_data[octave], 1);
			sizes[octave] = 0;
		}
		return;
	}
	
	// TODO: Make sure there is enough RAM for this!
	
	// These will become invalid, so we delete them to get ram for this operation
	delete_subsampled_versions();
	
	u32 new_n_samples = n_samples - (endsample - startsample);
	
	u8 bps;
	if(is_16_bit) bps=2; else bps=1;
	
	s8 *new_sounddata = (s8*)_malloc(bps * new_n_samples);
	// Copy the data before the deleted part
	memcpy(new_sounddata, sound_data[0], startsample*bps);
	// Copy the data after the deleted part
	memcpy(new_sounddata+startsample*bps, ((s8*)sound_data[0])+(endsample+1)*bps, n_samples*bps - endsample*bps);
	
	_free(sound_data[0]);
	sound_data[0] = new_sounddata;
	n_samples = new_n_samples;
	
	// Now everything's clear and we set the variables right and make the subsampled versions
	calcSize();
	loop_start = loop_length = 0;
	make_subsampled_versions();
}

void Sample::fadeIn(u32 startsample, u32 endsample)
{
	fade(startsample, endsample, true);
}

void Sample::fadeOut(u32 startsample, u32 endsample)
{
	fade(startsample, endsample, false);
}

void Sample::reverse(u32 startsample, u32 endsample)
{
	if(endsample >= n_samples)
		endsample = n_samples-1;
	
	// TODO: Make sure there is enough RAM for this!
	
	// These will become invalid, so we delete them to get ram for this operation
	delete_subsampled_versions();
	
	s32 offset = startsample;
	s32 length = endsample - startsample;
	
	if(is_16_bit == true) {
		s16 *new_sounddata = (s16*)_malloc(2 * length);
		s16 *sounddata = (s16*)(sound_data[0]);
		
		// First reverse the selected region
		for(s32 i=0;i<length;++i) {
			new_sounddata[i] = sounddata[offset+length-1-i];
		}
		
		// Then copy it into the sample
		for(s32 i=0;i<length;++i) {
			sounddata[offset+i] = new_sounddata[i];
		}
		
		_free(new_sounddata);
		
	} else {
		
		s8 *new_sounddata = (s8*)_malloc(length);
		s8 *sounddata = (s8*)(sound_data[0]);
		
		// First reverse the selected region
		for(s32 i=0;i<length;++i) {
			new_sounddata[i] = sounddata[offset+length-1-i];
		}
		
		// Then copy it into the sample
		for(s32 i=0;i<length;++i) {
			sounddata[offset+i] = new_sounddata[i];
		}
		
		_free(new_sounddata);
	}
	
	// Now everything's clear and we set the variables right and make the subsampled versions
	calcSize();
	loop_start = loop_length = 0;
	make_subsampled_versions();
}


void Sample::normalize(u8 percent)
{
	// These will become invalid, so we delete them to get ram for this operation
	delete_subsampled_versions();
	
	if(is_16_bit == true)
	{
		s16 *sounddata = (s16*)(sound_data[0]);
		s32 smp;
		
		for(u32 i=0;i<n_samples;++i) {
			smp = percent * sounddata[i] / 100;
			if(smp>32767)
				smp=32767;
			else if (smp<-32768)
				smp=32768;
			sounddata[i] = smp;
		}
		
	} else {
		
		s8 *sounddata = (s8*)(sound_data[0]);
		s16 smp;
		
		for(u32 i=0;i<n_samples;++i) {
			smp = percent * sounddata[i] / 100;
			if(smp>127)
				smp=127;
			else if(smp<-128)
				smp=-128;
			sounddata[i] = smp;
		}
	}
	
	// Now everything's clear and we set the variables right and make the subsampled versions
	make_subsampled_versions();
}

#endif

/* ===================== PRIVATE ===================== */

void Sample::calcSize(void)
{
	if(is_16_bit) {
	  sizes[0] = n_samples*2;
	} else {
	  sizes[0] = n_samples;
	}
}

#ifdef ARM9

void Sample::setFormat(void) {
	
	// TODO ADPCM and stuff
	if(is_16_bit) {
		sound_format = SOUND_16BIT;
	} else {
		sound_format = SOUND_8BIT;
	}
}

int fncompare (const void *elem1, const void *elem2 )
{
	if ( *(u16*)elem1 < *(u16*)elem2) return -1;
	else if (*(u16*)elem1 == *(u16*)elem2) return 0;
	else return 1;
}

// Takes the sampling rate in hz and searches for FT2-compatible values for
// finetune and rel_note in the freq_table
void Sample::calcRelnoteAndFinetune(u32 freq)
{
	u16 freqpos = findClosestFreq(freq);
	
	finetune = freqpos%N_FINETUNE_STEPS;
	rel_note = freqpos/N_FINETUNE_STEPS - BASE_NOTE;
}

// finds the freq in the freq table that is closest to freq ^^
u16 Sample::findClosestFreq(u32 freq)
{
	// Binary search!
	bool found = false;
	
	u16 left = 0, right = LINEAR_FREQ_TABLE_SIZE-1, middle = (right-left)/2 + left;
	if ( (linear_freq_table_lookup(middle) <= freq) && (linear_freq_table_lookup(middle+1) >= freq) ) {
		found = true;
	} else

		while(!found) {
			
			if(linear_freq_table_lookup(middle) < freq) {
				left = middle+1;
			} else {
				right = middle-1;
			}
			
			middle = (right-left)/2 + left;
			
			if ( (linear_freq_table_lookup(middle) <= freq) && (linear_freq_table_lookup(middle+1) > freq) ) {
				found = true;
			}
		}
		
	return middle;
}

bool Sample::make_subsampled_versions(void)
{
	u32 cursize = sizes[0] / 2;
	u16 curjump = 2;
	
	loop_starts[0]=loop_start;
	loop_lengths[0]=loop_length;

	for (u8 oct=1; oct<20; oct++) {
	  sound_data[oct]=0;
	  sizes[oct]=0;
	}

	if(is_16_bit)
	{
	  for(u8 octave = 1;octave<20;++octave)
	    {
	      loop_starts[octave]=loop_starts[octave-1]/2;
	      loop_lengths[octave]=loop_lengths[octave-1]/2;
	      
	      if (cursize==0) break; // no need for going on.
	      sound_data[octave] = _malloc(cursize);
	      if (!sound_data[octave]) return false;
	      sizes[octave] = cursize;
	      
	      s16 *srcsample = (s16*)sound_data[0];
	      s16 *destsample = (s16*)sound_data[octave];
	      for(u32 smp=0; smp<cursize/2; ++smp) {
		destsample[smp] = *srcsample;
		srcsample += curjump;
	      }
		  
	      cursize /= 2;
	      curjump *= 2;
	    }
		
	} else {
		
	  for(u8 octave = 1;octave<20;++octave)
	    {
	      loop_starts[octave]=loop_starts[octave-1]/2;
	      loop_lengths[octave]=loop_lengths[octave-1]/2;

	      if (cursize==0) break; // no need for going on.
	      sound_data[octave] = _malloc(cursize);
	      if (!sound_data[octave]) return false;
	      sizes[octave] = cursize;
			
	      s8 *srcsample = (s8*)sound_data[0];
	      s8 *destsample = (s8*)sound_data[octave];
	      for(u32 smp=0; smp<cursize; ++smp) {
		destsample[smp] = *srcsample;
		srcsample += curjump;
	      }
		  
	      cursize /= 2;
	      curjump *= 2;
	    }
		
	}
	return true;
}

void Sample::delete_subsampled_versions(void)
{
  if(sound_data[1] == 0)
    return;
  iprintf("deleting sub-samples ...\n");
  for(u8 i=1;i<20;++i) {

    if (sound_data[i]) { iprintf("#%p",sound_data[i]); _free(sound_data[i]); }
    sound_data[i] = 0;
  }
  iprintf(" done.\n");
}

void Sample::convertStereoToMono(void)
{
	if(is_16_bit == true) {
		// Make a buffer for the converted sample
		s16 *tmpbuf = (s16*)_malloc(sizes[0]);
		s16 *src = (s16*)sound_data[0];
		
		// Convert the sample down
		s32 smp;
		for(u32 i=0; i<sizes[0]/2; ++i) {
			smp = src[2*i] + src[2*i+1];
			tmpbuf[i] = smp / 2;
		}
		
		// Overwrite the original with the converted sample
		memcpy(sound_data[0], tmpbuf, sizes[0]);
		
		// Delete the temporary buffer
		_free(tmpbuf);
	} else {
		// Make a buffer for the converted sample
		s8 *tmpbuf = (s8*)_malloc(sizes[0]);
		s8 *src = (s8*)sound_data[0];
		
		// Convert the sample down
		s32 smp;
		for(u32 i=0; i<sizes[0]/2; ++i) {
			smp = src[2*i] + src[2*i+1];
			tmpbuf[i] = smp / 2;
		}
		
		// Overwrite the original with the converted sample
		memcpy(sound_data[0], tmpbuf, sizes[0]);
		
		// Delete the temporary buffer
		_free(tmpbuf);	
	}
}

void Sample::fade(u32 startsample, u32 endsample, bool in)
{
	if(endsample >= n_samples)
		endsample = n_samples-1;
	
	// TODO: Make sure there is enough RAM for this!
	
	// These will become invalid, so we delete them to get ram for this operation
	delete_subsampled_versions();
	
	s32 offset = startsample;
	s32 length = endsample - startsample + 1;
	
	if(is_16_bit == true) {
		s16 *new_sounddata = (s16*)_malloc(2 * length);
		s16 *sounddata = (s16*)(sound_data[0]);
		
		// First create a buffer with the faded sound data
		if(in==true) {
			for(s32 i=0;i<length;++i) {
				new_sounddata[i] = (((i * 1024) / length) * sounddata[offset+i]) / 1024;
			}
		} else {
			for(s32 i=0;i<length;++i) {
				new_sounddata[i] = ((((length-i) * 1024) / length) * sounddata[offset+i]) / 1024;
			}
		}
		
		// Then copy it into the sample
		for(s32 i=0;i<length;++i) {
			sounddata[offset+i] = new_sounddata[i];
		}
		
		_free(new_sounddata);
		
	} else {
		
		s8 *new_sounddata = (s8*)_malloc(length);
		s8 *sounddata = (s8*)(sound_data[0]);
		
		// First create a buffer with the faded sound data
		if(in==true) {
			for(s32 i=0;i<length;++i) {
				new_sounddata[i] = i * sounddata[offset+i] / length;
			}
		} else {
			for(s32 i=0;i<length;++i) {
				new_sounddata[i] = (length-i) * sounddata[offset+i] / length;
			}
		}
		
		// Then copy it into the sample
		for(s32 i=0;i<length;++i) {
			sounddata[offset+i] = new_sounddata[i];
		}
		
		_free(new_sounddata);
	}
	
	// Now everything's clear and we set the variables right and make the subsampled versions
	calcSize();
	loop_start = loop_length = 0;
	make_subsampled_versions();

}

// DS hardware requires length to be multiple of 4. 
// for sounds that have a very small (and odd) loop,
// this means the actual tuning frequency will not 
// match what's been expected by the author.
// we then create a modified version of the sample
// that 'unrolls' the loop until it has a correct size.
bool Sample::setupOddLoop(void)
{
  original_data = sound_data[0]; // "Backup"
  original_n_samples = n_samples;
  u32 original_size = loop_start+loop_length;
  u32 original_loop = loop_length;
  u32 newsize = sizes[0];
  
  while(loop_length&3) {
    loop_length+=original_loop;
    newsize+=original_loop;
  }
  iprintf("%s: %i..%i -> %i..%i\n",
	 name, (int)loop_start, (int)original_size, (int)loop_start, (int)loop_length);
  void *ptr = _realloc(pingpong_data, original_size + loop_length);
  if (ptr) pingpong_data=ptr;
  else return false;
  
  memcpy(pingpong_data, original_data, loop_start+original_loop);
  iprintf("copy duplicates ...\n");
  for(n_samples=original_size; n_samples < newsize ; n_samples+=original_loop) 
    memcpy(((char*)pingpong_data)+n_samples, 
	   ((char*)original_data)+loop_start, original_loop);
  
  sound_data[0] = pingpong_data;
  iprintf("update subsampled ...\n");
  if (is_16_bit) n_samples = newsize/2;
  else n_samples = newsize;

  calcSize();
  delete_subsampled_versions();
  make_subsampled_versions();
  iprintf("%s: all done",name);
  DC_FlushAll();
  return true;
}


// the DS sound hardware is not capable of ping-pong loops,
// so we actually "mirror" the samples beyond the loop-point
// and play the doubled loop as "forward loop".
void Sample::setupPingPongLoop(void)
{
	original_data = sound_data[0]; // "Backup"
	original_n_samples = n_samples;
	
	u32 original_size = sizes[0];
	
	pingpong_data = _realloc(pingpong_data, original_size + loop_length);
	
	// Copy sound data until loop end
	memcpy(pingpong_data, original_data, loop_start + loop_length);
	
	// Copy reverse loop
	if(is_16_bit)
	{
	  s16 *orig = (s16*)original_data;
	  s16 *pp = (s16*)pingpong_data;
	  u32 pos = (loop_start + loop_length) / 2;
		
	  for(u32 i=0; i<loop_length/2; ++i)
	    pp[pos+i] = orig[pos-i];
	}
	else
	{
	  s8 *orig = (s8*)original_data;
	  s8 *pp = (s8*)pingpong_data;
	  u32 pos = loop_start + loop_length;
	  
	  for(u32 i=0; i<loop_length; ++i)
	    pp[pos+i] = orig[pos-i];
	}
	
	// Copy rest
	u32 pos = loop_start + loop_length;
	memcpy((u8*)pingpong_data+ pos + loop_length, (u8*)original_data + pos, original_size - pos);
	
	// Set as new sound data
	sound_data[0] = pingpong_data;
	
	if(is_16_bit)
		n_samples = (original_size + loop_length) / 2;
	else
		n_samples = original_size + loop_length;
	
	calcSize();
	delete_subsampled_versions();
	make_subsampled_versions();
	
	DC_FlushAll();
}

void Sample::removePingPongLoop(void)
{
	_free(pingpong_data);
	pingpong_data = 0;
	
	sound_data[0] = original_data;
	n_samples = original_n_samples;
	
	/*	if(is_16_bit)      // this is calcSize!
	  sizes[0] = n_samples*2;
	  else
	  sizes[0] = n_samples;
	*/
	calcSize();
	
	delete_subsampled_versions();
	
	make_subsampled_versions();
	
	
	DC_FlushAll();
}

void Sample::updatePingPongLoop(void)
{
	if(pingpong_data != 0)
		removePingPongLoop();
	setupPingPongLoop();
}

#endif
