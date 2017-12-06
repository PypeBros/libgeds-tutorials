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

#ifndef WAV_H
#define WAV_H

#define CMP_PCM		0
#define CMP_ADPCM		1

#define MAX_WAV_SIZE (1024*1024)

#include <nds.h>

/*

A class that imeplements wav reading
and writing exactly as far as we need
it and not one single bit more.

It supports
- 8/16 Bit
- arbitrary sampling rate

*/

class Wav {
	public:
		Wav();
		~Wav();
		bool load(const char *filename);
		bool save(const char *filename);
		
		u8 *getAudioData(void)    { return audio_data_; }
		u32 getNSamples(void)     { return (n_channels_==2)?n_samples_/2:n_samples_; }
		u16 getSamplingRate(void) { return sampling_rate_; }
		bool isStereo(void)       { return n_channels_ == 2; }
		u8 getBitPerSample(void)  { return bit_per_sample_; }
		u8 getCompression(void)   { return compression_; }

	private:
		u8 compression_;
		u8 n_channels_;
		u16 sampling_rate_;
		u8 bit_per_sample_;
		u32 n_samples_;
		u8 *audio_data_;
};

#endif
