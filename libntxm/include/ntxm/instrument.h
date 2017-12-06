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

#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include "sample.h"

#define MAX_OCTAVE		9
#define MAX_NOTE	(MAX_OCTAVE*12-1)

#define INST_SAMPLE	0
#define INST_SYNTH	1 // Yes, who knows!

#define MAX_INST_NAME_LENGTH	22

#define MAX_ENV_X			324
#define MAX_ENV_Y			64

struct ChannelState;

class Instrument : NtMagic
{
	friend class EnvelopeEditor;
	friend class XMTransport;
	
	public:
	
		Instrument(const char *_name, u8 _type=INST_SAMPLE, u8 _volume=255);
		Instrument(const char *_name, Sample *_sample, u8 _volume=255);
		~Instrument();
	
		void addSample(Sample *sample);
		Sample *getSample(u8 idx); // If not present, 0 is returned
		void setSample(u8 idx, Sample *sample);
		Sample *getSampleForNote(u8 _note);
		// void advance(u8 note, long offset, u8 _channel);
		void play(u8 _note, u8 _volume, struct ChannelState* _channel);
		// void bendNote(u8 _note, u8 _finetune, ChannelState *_channel);
		void setNoteSample(u16 note, u8 sample_id);
		u8 getNoteSample(u16 note);
		void setVolEnvEnabled(bool is_enabled);
		bool getVolEnvEnabled(void);
		
		// Calculate how long in ms the instrument will play note given note
		u32 calcPlayLength(u8 note);
		
		const char *getName(void);
		void setName(const char *_name);
	
		u16 getSamples(void);
		
		void setVolumeEnvelope(u16 *envelope, u8 n_points, bool vol_env_on_, bool vol_env_sustain_, bool vol_env_loop_);
		void setPanningEnvelope(u16 *envelope, u8 n_points, bool pan_env_on_, bool pan_env_sustain_, bool pan_env_loop_);
		
		void updateEnvelopePos(u8 bpm, u8 ms_passed);
		u16 getEnvelopeAmp(void);
		
	private:
		
		char name[MAX_INST_NAME_LENGTH+1];
		
		u8 type;
		u8 volume;
		
		// Actually this would become dirty if more types emerge.
		// I think making an abstract Instrument base class that
		// subclasses SampleInstrument and SynthInstrument would
		// be derived from would be the best choice. But I'll do
		// this when required ^^
	
		Sample **samples;
		u16 n_samples;
	
		// Synth *synth;
	
		u8 note_samples[MAX_OCTAVE*12];
		
		u16 vol_envelope_x[12];
		u16 vol_envelope_y[12];
		u8 n_vol_points;
		bool vol_env_on;
		bool vol_env_sustain;
		bool vol_env_loop;
		
		u16 pan_envelope_x[12];
		u16 pan_envelope_y[12];
		u8 n_pan_points;
		bool pan_env_on;
		bool pan_env_sustain;
		bool pan_env_loop;
		
		u16 envelope_ms;
		u16 envelope_pixels; // Pixel of the FT2 envelope editor :-)
};

#endif
