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

#include "ntxm/instrument.h"

#ifdef ARM9
#include <stdio.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "tools.h"

#include "command.h"
#include "ntxm/channel.h"
#ifdef ARM9
#include "tools.h"
#endif

#ifdef ARM9

Instrument::Instrument(const char *_name, u8 _type, u8 _volume)
	:type(_type), volume(_volume),
	 n_vol_points(0), vol_env_on(true), vol_env_sustain(false), vol_env_loop(false),
	 n_pan_points(0)
{
	name = (char*)calloc(MAX_INST_NAME_LENGTH+1, 1);
	
	my_strncpy(name, _name, MAX_INST_NAME_LENGTH);
	
	note_samples = (u8*)calloc(sizeof(u8)*MAX_OCTAVE*12, 1);
	
	samples = NULL;
	n_samples = 0;
}

Instrument::Instrument(const char *_name, Sample *_sample, u8 _volume)
	:type(INST_SAMPLE), volume(_volume)
{
	name = (char*)malloc(MAX_INST_NAME_LENGTH+1);
	for(u16 i=0; i<MAX_INST_NAME_LENGTH+1; ++i) name[i] = '\0';
	my_strncpy(name, _name, MAX_INST_NAME_LENGTH);
	
	samples = (Sample**)malloc(sizeof(Sample*)*1);
	samples[0] = _sample;
	n_samples = 1;
	
	note_samples = (u8*)malloc(sizeof(u8)*MAX_OCTAVE*12);
	for(u16 i=0;i<MAX_OCTAVE*12; ++i)
		note_samples[i] = 0;
}

Instrument::~Instrument()
{
	for(u8 i=0;i<n_samples;++i)
		delete samples[i];
	
	if(samples != NULL)
		free(samples);
	
	free(note_samples);
	free(name);
}

void Instrument::addSample(Sample *sample)
{
	n_samples++;
	samples = (Sample**)realloc(samples, sizeof(Sample*)*n_samples);
	samples[n_samples-1] = sample;
}

void Instrument::setSample(u8 idx, Sample *sample)
{
	// Delete the sample if it already exists
	if( (idx < n_samples) && (samples[idx] != 0) )
		delete samples[idx];
	
	// Resize sample list if necessary
	if(n_samples < idx + 1)
	{
		samples = (Sample**)realloc(samples, sizeof(Sample*) * (idx + 1));
		
		// Initialize new samples with 0
		while(n_samples < idx + 1)
		{
			samples[n_samples] = 0;
			++n_samples;
		}
	}
	
	samples[idx] = sample;
}

#endif

Sample *Instrument::getSample(u8 idx)
{
	if((n_samples>0) && (idx<n_samples))
		return samples[idx];
	else
		return NULL;
}

Sample *Instrument::getSampleForNote(u8 _note) {
	return samples[note_samples[_note]];
}

#ifdef ARM7

/*void Instrument::advance(u8 _note, long offset, u8 _channel)
{
  if (n_samples>0 && type==INST_SAMPLE)
    samples[note_samples[_note]]->advance(_note,offset, _channel);
}*/

void Instrument::play(u8 _note, u8 _volume, struct ChannelState* _channel)
{
  envelope_ms = 0;
  envelope_pixels = 0;
  
  if(_note > MAX_NOTE) {
    //CommandDbgOut("Note (%u) > MAX_NOTE (%u)\n",_note,MAX_NOTE);
    return;
  }
  
  u8 play_volume = volume * _volume / 255;
  
  if((vol_env_on)&&(n_vol_points>0))
    play_volume = play_volume * vol_envelope_y[0] / 64;
  
  switch(type) {
  case INST_SAMPLE:
    if(n_samples > 0) {
      _channel->smp = samples[note_samples[_note]];
      _channel->smp->play(_note, play_volume, _channel);
    }
    break;
  }
}

#ifdef __DEPRECATED__
void Instrument::bendNote(u8 _note, u8 _finetune, ChannelState* _channel)
{
  if(_note > MAX_NOTE)
    return;
  
  switch(type) {
  case INST_SAMPLE:
    if(n_samples > 0)
      samples[note_samples[_note]]->bendNote(_note, _finetune, _channel);
    break;
  }
}
#endif

#endif

#ifdef ARM9

void Instrument::setNoteSample(u16 note, u8 sample_id) {
	note_samples[note] = sample_id;
}

#endif

u8 Instrument::getNoteSample(u16 note) {
	return note_samples[note];
}

#ifdef ARM9

void Instrument::setVolEnvEnabled(bool is_enabled)
{
	vol_env_on = is_enabled;
	DC_FlushAll();
}

#endif

bool Instrument::getVolEnvEnabled(void)
{
	return vol_env_on;
}

// Calculate how long in ms the instrument will play note given note
u32 Instrument::calcPlayLength(u8 note) {
	return samples[note_samples[note]]->calcPlayLength(note);
}

#ifdef ARM9

const char *Instrument::getName(void) {
	return name;
}

void Instrument::setName(const char *_name) {
	my_strncpy(name, _name, MAX_INST_NAME_LENGTH);
}

#endif

u16 Instrument::getSamples(void) {
	return n_samples;
}

#ifdef ARM9

void Instrument::setVolumeEnvelope(u16 *envelope, u8 n_points, bool vol_env_on_, bool vol_env_sustain_, bool vol_env_loop_)
{
	n_vol_points = n_points;
	for(u8 i=0; i<n_points; ++i)
	{
		vol_envelope_x[i] = envelope[2*i];
		vol_envelope_y[i] = envelope[2*i+1];
	}
	
	vol_env_on      = vol_env_on_;
	vol_env_sustain = vol_env_sustain_;
	vol_env_loop    = vol_env_loop_;
}

void Instrument::setPanningEnvelope(u16 *envelope, u8 n_points, bool pan_env_on_, bool pan_env_sustain_, bool pan_env_loop_)
{
	n_pan_points = n_points;
	for(u8 i=0; i<n_points; ++i)
	{
		pan_envelope_x[i] = envelope[2*i];
		pan_envelope_y[i] = envelope[2*i+1];
	}
	
	pan_env_on      = pan_env_on_;
	pan_env_sustain = pan_env_sustain_;
	pan_env_loop    = pan_env_loop_;
}

#endif

void Instrument::updateEnvelopePos(u8 bpm, u8 ms_passed)
{
	envelope_ms += ms_passed;
	envelope_pixels = envelope_ms * bpm * 50 / 120 / 1000; // 50 pixels per second at 120 BPM
}

u16 Instrument::getEnvelopeAmp(void)
{
	if( (n_vol_points == 0) || (vol_env_on == false) )
		return 64;
	
	u8 envpoint = 0;
	while( (envpoint < n_vol_points - 1) && (envelope_pixels >= vol_envelope_x[envpoint+1]) )
		envpoint++;
	
	if(envpoint >= n_vol_points - 1) // Last env point?
		return vol_envelope_y[envpoint];
	
	u16 x1 = vol_envelope_x[envpoint];
	u16 y1 = vol_envelope_y[envpoint];
	u16 x2 = vol_envelope_x[envpoint+1];
	u16 y2 = vol_envelope_y[envpoint+1];
	
	u16 rel_x = envelope_pixels - x1;
	
	u16 y = y1 + rel_x * (y2 - y1) / (x2 - x1);
	if(y > 64)
		y = 64;
	
	return y;
}
