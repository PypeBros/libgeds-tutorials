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

#include "wav.h"

#if defined(ARM9)
#include "tools.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <nds.h>

#if defined(ARM9)

#include <fat.h>
#endif

/* ===================== PUBLIC ===================== */

Wav::Wav()
	:compression_(CMP_PCM), n_channels_(1), sampling_rate_(22050), bit_per_sample_(8),
	n_samples_(0), audio_data_(0)
{

}

Wav::~Wav() {
	
}

bool Wav::load(const char *filename)
{
#if defined(ARM9)
	// Init
	
	if(!myInitFiles())
		return false;
	
	FILE *fileh;
	
	fileh = fopen(filename, "r");
	
	if((s32)fileh == -1)
		return false;
	
	// Check if the file is not too big
	fseek(fileh, 0, SEEK_END);
	u32 filesize = ftell (fileh);
	
	if(filesize > MAX_WAV_SIZE) {
		fclose(fileh);
		//iprintf("file too big\n");
		return false;
	}
	
	if(filesize == 0) {
		fclose(fileh);
		//iprintf("0-byte file!\n");
		return false;
	}
	
	fseek(fileh, 0, SEEK_SET);
	
	char *buf = (char*)malloc(5);
	memset(buf,0,5);
	
	// RIFF header
	fread(buf, 1, 4, fileh);
	
	if(strcmp(buf,"RIFF")!=0) {
		fclose(fileh);
		return false;
	}
	
	u32 riff_size;
	fread(&riff_size, 4, 1, fileh);
	
	// WAVE header
	fread(buf, 1, 4, fileh);
	if(strcmp(buf,"WAVE")!=0) {
		fclose(fileh);
		return false;
	}
	
	// fmt chunk
	fread(buf, 1, 4, fileh);
	if(strcmp(buf,"fmt ")!=0) {
		fclose(fileh);
		return false;
	}
	
	u32 fmt_chunk_size;
	fread(&fmt_chunk_size, 4, 1, fileh);
	
	u16 compression_code;
	fread(&compression_code, 2, 1, fileh);
	
	// 1 : pcm, 17 : ima adpcm
	if(compression_code == 1) {
		compression_ = CMP_PCM;
	} /*else if(compression_code == 17) {
		compression_ = CMP_ADPCM;
	}*/ else {
		fclose(fileh);
		return false;
	}
	
	u16 n_channels; // They really thought they were cool when making this 16 bit.
	fread(&n_channels, 2, 1, fileh);
	
	if(n_channels > 2) {
		fclose(fileh);
		return false;
	} else {
		n_channels_ = n_channels;
	}
	
	u32 sampling_rate;
	fread(&sampling_rate, 4, 1, fileh);
	
	sampling_rate_ = sampling_rate;
	
	u32 avg_bytes_per_sec; // We don't need this
	fread(&avg_bytes_per_sec, 4, 1, fileh);
	
	u16 block_align; // We don't need this
	fread(&block_align, 2, 1, fileh);
	
	u16 bit_per_sample;
	fread(&bit_per_sample, 2, 1, fileh);
	if((bit_per_sample==8)||(bit_per_sample==16)) {
		bit_per_sample_ = bit_per_sample;
	}
	
	// Skip extra bytes
	fseek(fileh, fmt_chunk_size - 16, SEEK_CUR);
	
	fread(buf, 1, 4, fileh);
	
	// Skip forward to the data chunk
	while((!feof(fileh))&&(strcmp(buf,"data")!=0)) {
		u32 chunk_size;
		fread(&chunk_size, 4, 1, fileh);
		fseek(fileh, chunk_size, 1);
		fread(buf, 1, 4, fileh);
	}
	
	if(feof(fileh)) {
		fclose(fileh);
		return false;
	}
	
	// data chunk
	u32 data_chunk_size;
	fread(&data_chunk_size, 4, 1, fileh);
	
	u8 sample_size = bit_per_sample_/8;
	if(compression_ == CMP_PCM) {
		n_samples_ = data_chunk_size / sample_size;
	} else {
		n_samples_ = data_chunk_size * 2 * sample_size;
	}
	
	audio_data_ = (u8*)malloc(data_chunk_size);
	memset(audio_data_, 0, data_chunk_size);
	
	if(audio_data_ == 0) {
		//iprintf("Could not alloc mem for wav.\n");
		free(buf);
		fclose(fileh);
		return false;
	}
	
	// Read the data
	fread(audio_data_, data_chunk_size, 1, fileh);
	
	// Convert 8 bit samples from unsigned to signed
	if(bit_per_sample == 8) {
		s8* signed_data = (s8*)audio_data_;
		for(u32 i=0; i<data_chunk_size; ++i) {
			signed_data[i] = audio_data_[i] - 128;
		}
	}
	
	// Finish up
	free(buf);
	
	fclose(fileh);
	
#endif
	return true;
}

bool Wav::save(const char *filename) {
	return true; // Hehe
}

/* ===================== PRIVATE ===================== */
