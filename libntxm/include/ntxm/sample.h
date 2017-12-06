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

#ifndef SAMPLE_H
#define SAMPLE_H

#include <nds.h>
#include "linear_freq_table.h"

#define BASE_NOTE			96	// Index if C-4 (FT2 base note)
#define SILENCE_THRESHOLD_16	2000
#define CROP_IGNORE_START	200

#define NO_VOLUME			255

#define NO_LOOP			0
#define FORWARD_LOOP		1
#define PING_PONG_LOOP		2

#define SAMPLE_NAME_LENGTH	24

struct ChannelState;

class NtMagic {
  u32 __magic;
 protected:
  NtMagic(const char* name) 
    : __magic(name[0] + (name[1]<<8) + (name[2]<<16) + (name[3]<<24))
    { }
    virtual ~NtMagic() { 
      __magic|=0xdead;
    }
};

class Sample : NtMagic
{
  
  /*  Sample(const Sample& s) {
  // illegal use of copy constructor.
  }
  Sample operator=(const Sample& s) {
  // illegal use of copy constructor.
  }*/
 public:
  Sample(void *_sound_data, u32 _n_samples, u16 _sampling_frequency=44100,
	 bool _is_16_bit=true, u8 _loop=NO_LOOP, u8 _volume=255);


  ~Sample();
  
  void advance(long offset, struct ChannelState* _channel);
  void play(u8 note, u8 volume_, struct ChannelState* channel  /* effects here */);
  void bendNote(u8 note, u8 finetune, struct ChannelState* channel);
  u32 calcPlayLength(u8 note);
  
  void setRelNote(s8 _rel_note);
  void setFinetune(s8 _finetune);
  
  u8 getRelNote(void);
  s8 getFinetune(void);
  
  u32 getSize(void);
  u32 getNSamples(void);
  
  void *getData(void);
  
  u8 getLoop(void); // 0: no loop, 1: loop, 2: ping pong loop
  bool setLoop(u8 loop_); // Set loop type. Can fail due to memory constraints
  bool is16bit(void);
  
  void setLoopLength(u32 _loop_length);
  u32 getLoopLength(void);
  void setLoopStart(u32 _loop_start);
  u32 getLoopStart(void);
  void setLoopStartAndLength(u32 _loop_start, u32 _loop_length);
  
  void setVolume(u8 vol);
  u8 getVolume(void);
  
  void setName(const char *name_);
  const char *getName(void);
  
  // Deletes the part between start sample and end sample
  void delPart(u32 startsample, u32 endsample);
  
  void fadeIn(u32 startsample, u32 endsample);
  void fadeOut(u32 startsample, u32 endsample);
  void reverse(u32 startsample, u32 endsample);
  void normalize(u8 percent);
  
  //void cutSilence(void); // Heuristically cut silence in the beginning
  
 private:
  void calcSize(void);
  void setFormat(void);
  void calcRelnoteAndFinetune(u32 freq);
  u16 findClosestFreq(u32 freq);
  bool make_subsampled_versions(void);
  void delete_subsampled_versions(void);
  void convertStereoToMono(void);
  
  void fade(u32 startsample, u32 endsample, bool in);
  
  void setupPingPongLoop(void);
  void removePingPongLoop(void);
  void updatePingPongLoop(void);
  bool setupOddLoop(void);
  //void subsample_with_lowpass(void *src, void *dest, bool _16bit, u32 size);
  //void subsample(void *src, void *dest, bool _16bit, u32 size);
  
  // Because of the incapabilty of the DS to play samples at notes higher
  // than F-7 the sample class stores subsampled versions of the
  // sample and plays these if notes above their basenotes are required.
  void *sound_data[20];
  void *original_data;
  void *pingpong_data;
  u32 n_samples;
  u32 original_n_samples;
  bool is_16_bit;
  u8 loop;
  s8 rel_note;		// Offset in the frequency table from base note
  s8 finetune;		// -128: one halftone down, +127: one halftone up
  u32 loop_start;		// In bytes, not in samples!
  u32 loop_length;		// In bytes, not in samples!
  u8 volume;
  char name[SAMPLE_NAME_LENGTH];
  
  // These are calculated in the constructor
  u32 sizes[20];
  u32 loop_starts[20];
  u32 loop_lengths[20];
  u32 sound_format;
};

#endif
