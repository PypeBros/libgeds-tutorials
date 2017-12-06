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

#include "ntxm/xm_transport.h"

#include "tools.h"
#include "ntxm/datareader.h"
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
// DBG
#include <stdio.h>
#include <nds.h>
// DBG

char xmtransporterrors[][100] =
	{"fat init failed",
	"could not open file",
	"not a valid xm file",
	"memory full",
	"pattern read error",
	"file too big for ram",
	"",
	"pattern too long",
	 "file is zero byte"};

/* ===================== PUBLIC ===================== */

// Loads a song from a file and puts it in the song argument
// returns 0 on success, an error code else
u16 XMTransport::load(DataReader *file, Song **_song)
{
	//
	// Init
	//
/*
       if(!myInitFiles())
	return XM_TRANSPORT_ERROR_INITFAIL;
*/	
//	FILE *xmfile = fopen(filename, "r");
	
//	if((s32)xmfile == -1)
//		return XM_TRANSPORT_ERROR_FOPENFAIL;
	
	// Check if the song fits into RAM
  unsigned filesize = file->tellsize();
  if(filesize > MAX_XM_FILESIZE)
    return XM_TRANSPORT_FILE_TOO_BIG_FOR_RAM;
  
  if(filesize == 0 || file->gotwrong) 
    return XM_TRANSPORT_FILE_ZERO_BYTE;

	
  //
  // Read header
  //
  
  // Magic number
  char magicnumber[18] = {0};
  file->read(magicnumber, 17);
  
  if( strcmp(magicnumber, "Extended Module: ") != 0 ) 
    return XM_TRANSPORT_ERROR_MAGICNUMBERINVALID;

  // Song name
  char songname[21] = {0};
  file->read(songname, 20);
  
  // Skip uninteresting stuff like tracker name and version
  file->skip(23);
	
  // Header size
  u32 header_size=file->U32();

  // Song length (pot size)
  u16 pot_size=file->U16();
	
  // Restart position
  u16 restart_pos=file->U16();
	
  // Number of channels
  u16 n_channels=file->U16();

  // Number of patterns
  u16 n_patterns=file->U16();
	
  // Number of instruments
  u16 n_inst=file->U16();
	
  // Flags, currently only used for the frequency table (0: amiga, 1: linear)
  // TODO: Amiga freq table
  u16 flags=file->U16();

  // Tempo
  u16 tempo=file->U16();
  if(tempo == 0) tempo = 1; // Found an XM that actually had 0 there
  
  // BPM
  u16 bpm=file->U16();

  if (file->gotwrong) return 2;
  
  // Construct the song with the current info
  Song *song = new Song(tempo, bpm, n_channels);
  song->setName(songname);
  song->setRestartPosition(restart_pos);
  
  // Pattern order table
  u8 i/*, potentry*/;
  
  //fread(&potentry, 1, 1, xmfile);
  song->setPotEntry(0,file->U8()); // The first entry is made automatically by the song
  
  for(i=1;i<pot_size;++i) {
    //fread(&potentry, 1, 1, xmfile);
    song->potAdd(file->U8());
  }
  file->skip(256-pot_size);
  if (file->gotwrong) return 2;
  
  //
  // Read patterns
  //
  
  u8 pattern;
  for(pattern=0;pattern<n_patterns;++pattern) {
    // Pattern header length
    volatile u32 pattern_header_length=file->U32();
    //  fread(&pattern_header_length, 4, 1, xmfile);
    
    // Skip packing type (is always 0)
    file->skip(1);
    
    // Number of rows
    u16 n_rows=file->U16();
		
    if(n_rows > MAX_PATTERN_LENGTH)
      return XM_TRANSPORT_PATTERN_TOO_LONG;
    
    // Packed patterndata size
    u16 patterndata_size=file->U16();
    //TODO: Handle empty patterns (which are left out in the xm format)
    
    if(!file->gotwrong && patterndata_size > 0) { // Read the pattern
      u8 *ptn_data = (u8*)_malloc(patterndata_size);
      u32 bytes_read;
      bytes_read = file->read(ptn_data, patterndata_size);
      
      if(bytes_read != patterndata_size) 
	return XM_TRANSPORT_ERROR_PATTERN_READ;
      
      u32 ptn_data_offset = 0;
      u8 chn;
      u16 row;

      if(pattern>0) {
	song->addPattern();
      }
      
      song->resizePattern(pattern, n_rows);
      Cell **ptn = song->getPattern(pattern);
      
      for(row=0;row<n_rows;++row) {
	for(chn=0;chn<n_channels;++chn)	{
	  u8 magicbyte = 0, note = EMPTY_NOTE, inst = NO_INSTRUMENT, vol = NO_VOLUME,
	    eff_type = NO_EFFECT, eff_param = NO_EFFECT_PARAM, eff2_type = NO_EFFECT,
	    eff2_param = NO_EFFECT_PARAM;
	  
	  magicbyte = ptn_data[ptn_data_offset];
	  ptn_data_offset++;
	  
	  bool read_note=true, read_inst=true, read_vol=true,
	    read_eff_type=true, read_eff_param=true;
	  
	  if(magicbyte & 1<<7) { // It's the magic byte!
	    read_note = magicbyte & 1;
	    read_inst = magicbyte & 2;
	    read_vol = magicbyte & 4;
	    read_eff_type = magicbyte & 8;
	    read_eff_param = magicbyte & 16;
	    note = 0;
	  } else { // It's the note!
	    note = magicbyte;
	    read_note = false;
	    
	  }
	  
	  if(read_note) {
	    note = ptn_data[ptn_data_offset];
	    ptn_data_offset++;
	  } 
	  
	  if(read_inst) {
	    inst = ptn_data[ptn_data_offset];
	    ptn_data_offset++;
	  } 
	  
	  if(read_vol) {
	    vol = ptn_data[ptn_data_offset];
	    ptn_data_offset++;
	  } else {
	    vol = 0; // 'Do nothing'
	  }
	  
	  if(read_eff_type) {
	    eff_type = ptn_data[ptn_data_offset];
	    ptn_data_offset++;
	  } else {
	    if(!read_eff_param)
	      eff_type = NO_EFFECT;
	    else
	      eff_type = EFFECT_ARPEGGIO; // If we have params, but no effect, assume arpeggio
	  }
	  
	  if(read_eff_param) {
	    eff_param = ptn_data[ptn_data_offset];
	    ptn_data_offset++;
	  } 
	  
	  // Insert note into song
	  if(note==0) {
	    ptn[chn][row].note = EMPTY_NOTE;
	  } else if(note==97) {
	    ptn[chn][row].note = STOP_NOTE;
	  } else {
	    ptn[chn][row].note = note - 1;
	  }
	  
	  if(inst != NO_INSTRUMENT) {
	    ptn[chn][row].instrument = inst-1; // XM Inst indices start with 1
	  } else {
	    ptn[chn][row].instrument = NO_INSTRUMENT;
	  }
	  
	  // Separate volume column effects from the volume column
	  // and put them into the effcts column instead
	  
	  if((vol >= 0x10) && (vol <= 0x50))
	    {
	      u16 volume = (vol-16)*2;
	      if(volume>=MAX_VOLUME) volume = MAX_VOLUME;
	      if (note!=0) ptn[chn][row].volume = volume;
	      else {
		eff2_type = EFFECT_SET_VOLUME;
		eff2_param= vol-16;
	      }
	    }
	  else if(vol==0)
	    {
	      ptn[chn][row].volume = NO_VOLUME;
	    }
	  else if(vol>=0x60)
	    {
	      // It's an effect!
	      u8 volfx_param = vol & 0x0F;
	      
	      if( (vol>=0x60)&&(vol<=0x6F) ) { // Volume slide down
		eff2_type = EFFECT_VOLUME_SLIDE;
		eff2_param = volfx_param;
	      } else if( (vol>=0x70)&&(vol<=0x7F) ) { // Volume slide up
		eff2_type = EFFECT_VOLUME_SLIDE;
		eff2_param = volfx_param << 4;
	      } else if( (vol>=0x80)&&(vol<=0x8F) ) { // Fine volume slide down
		eff2_type = EFFECT_E;
		eff2_param = 0xB0 | volfx_param;
	      } else if( (vol>=0x90)&&(vol<=0x9F) ) { // Fine volume slide up
		eff2_type = EFFECT_E;
		eff2_param = 0xA0 | volfx_param;
	      } else if( (vol>=0xA0)&&(vol<=0xAF) ) { // Set vibrato speed (calls vibrato)
		eff2_type = EFFECT_VIBRATO;
		eff2_param = volfx_param << 4;
	      } else if( (vol>=0xB0)&&(vol<=0xBF) ) { // Vibrato
		eff2_type = EFFECT_VIBRATO;
		eff2_param = volfx_param; // Vibrato depth
	      } else if( (vol>=0xC0)&&(vol<=0xCF) ) { // Set panning
		eff2_type = 0x08;
		eff2_param = volfx_param << 4;
	      } else if( (vol>=0xD0)&&(vol<=0xDF) ) { // Panning slide left
		eff2_type = 0x19;
		eff2_param = volfx_param;
	      } else if( (vol>=0xD0)&&(vol<=0xDF) ) { // Panning slide right
		eff2_type = 0x19;
		eff2_param = volfx_param << 4;
	      } else if( vol>=0xF0 ) { // Tone porta
		eff2_type = EFFECT_PORTA_NOTE;
		eff2_param = volfx_param << 4;
	      }
	    }
	  
	  ptn[chn][row].effect = eff_type;
	  ptn[chn][row].effect_param = eff_param;
	  ptn[chn][row].effect2 = eff2_type;
	  ptn[chn][row].effect2_param = eff2_param;
	}
	
      }
      
      _free(ptn_data);
      
    } else { // Make an empty pattern
      
      if(pattern > 0) {
	song->addPattern();
      }
      
      song->resizePattern(pattern, n_rows);
    }
    
  }
  
  //
  // Read instruments
  //
  
  for(u8 inst=0; inst<n_inst; ++inst)
    {
      struct InstInfo *instinfo = (struct InstInfo*)_calloc(sizeof(struct InstInfo), 1);
      
      // Read fields up to number of samples
      instinfo->inst_size=file->U32();
      file->read(&instinfo->name, 22);
      instinfo->inst_type=file->U8();
      instinfo->n_samples=file->U16();
      
      Instrument *instrument = new Instrument(instinfo->name);
      if(instrument == 0)
	return XM_TRANSPORT_ERROR_MEMFULL;
      
      song->setInstrument(inst, instrument);
      
      if(instinfo->n_samples > 0) {
	// Read the rest of the instrument info
	instinfo->sample_header_size=file->U32();
	file->read(&instinfo->note_samples, 96);
	file->read( &instinfo->vol_points, 48);
	file->read( &instinfo->pan_points, 48);
	instinfo->n_vol_points=file->U8();
	instinfo->n_pan_points=file->U8();
	instinfo->vol_sustain_point=file->U8();
	instinfo->vol_loop_start_point=file->U8();
	instinfo->vol_loop_end_point=file->U8();
	instinfo->pan_sustain_point=file->U8();
	instinfo->pan_loop_start_point=file->U8();
	instinfo->pan_loop_end_point=file->U8();
	instinfo->vol_type=file->U8();
	instinfo->pan_type=file->U8();
	instinfo->vibrato_type=file->U8();
	instinfo->vibrato_sweep=file->U8();
	instinfo->vibrato_depth=file->U8();
	instinfo->vibrato_rate=file->U8();
	instinfo->vol_fadeout=file->U16();
	file->read( &instinfo->reserved_bytes, 11);
	
	bool vol_env_on, vol_env_sustain, vol_env_loop, pan_env_on, pan_env_sustain, pan_env_loop;
	vol_env_on      = instinfo->vol_type & BIT(0);
	vol_env_sustain = instinfo->vol_type & BIT(1);
	vol_env_loop    = instinfo->vol_type & BIT(2);
	pan_env_on      = instinfo->pan_type & BIT(0);
	pan_env_sustain = instinfo->pan_type & BIT(1);
	pan_env_loop    = instinfo->pan_type & BIT(2);
	
	instrument->setVolumeEnvelope(instinfo->vol_points, instinfo->n_vol_points,
				      vol_env_on, vol_env_sustain, vol_env_loop);
	instrument->setPanningEnvelope(instinfo->pan_points, instinfo->n_pan_points, 
				       pan_env_on, pan_env_sustain, pan_env_loop);
	
	// Skip the rest of the header if is longer than the current position
	// This was really strange and took some time (and debugging with Tim)
	// to figure out. Why the fsck is the instrument header that much longer?
	// Well, don't care, skip it.
	if(instinfo->inst_size > 252)
	  file->skip(instinfo->inst_size-252);
	
	for(u8 i=0; i<96; ++i)
	  instrument->setNoteSample(i,instinfo->note_samples[i]);
	
	// Load the sample(s)
	
	// Headers
	u8 *sample_headers = (u8*)_malloc(instinfo->n_samples*40);
	file->read(sample_headers, 40*instinfo->n_samples);
	
	for(u8 sample_id=0; sample_id < instinfo->n_samples; sample_id++)
	  {
	    // Sample length
	    u32 sample_length;
	    sample_length = *(u32*)(sample_headers+40*sample_id + 0);
	    
	    // Sample loop start
	    u32 sample_loop_start;
	    sample_loop_start = *(u32*)(sample_headers+40*sample_id + 4);
	    
	    // Sample loop length
	    u32 sample_loop_length;
	    sample_loop_length = *(u32*)(sample_headers+40*sample_id + 8);
	    
	    // Volume (0-64)
	    u8 sample_volume;
	    sample_volume = *(u8*)(sample_headers+40*sample_id + 12);
	    
	    if(sample_volume == 64) { // Convert scale to 0-255
	      sample_volume = 255;
	    } else {
	      sample_volume *= 4;
	    }
	    
	    // Finetune
	    s8 sample_finetune;
	    sample_finetune = *(s8*)(sample_headers+40*sample_id + 13);
	    
	    // Type byte (loop type and wether it's 8 or 16 bit)
	    u8 sample_type;
	    sample_type = *(u8*)(sample_headers+40*sample_id + 14);
	    
	    u8 loop_type = sample_type & 3;
	    
	    bool sample_is_16_bit = sample_type & 0x10;
	    
	    // Panning
	    u8 sample_panning;
	    sample_panning = *(u8*)(sample_headers+40*sample_id + 15);
	    
	    // Relative note
	    s8 sample_rel_note;
	    sample_rel_note = *(s8*)(sample_headers+40*sample_id + 16);
	    
	    // Sample name
	    char sample_name[23];
	    memset(sample_name, 23, 0);
	    memcpy(sample_name, sample_headers+40*sample_id + 18, 22);
	    
	    // Cut off trailing spaces
	    u8 i=21;
	    while(sample_name[i] == ' ')
	      --i;
	    ++i;
	    sample_name[i] = '\0';
	    
	    // Sample data
	    if (sample_length>0) {
	      void *sample_data = _malloc(sample_length);
	      
	      if(sample_data==NULL) {
		iprintf("couldn' allocate %u bytes for sample", sample_length);
		return XM_TRANSPORT_ERROR_MEMFULL;
	      }
	      
	      file->read(sample_data, sample_length);
	      
	      // Delta-decode
	      if(sample_is_16_bit)
		{
		  s16 last = 0;
		  s16 *smp = (s16*)sample_data;
		  for(u32 i=0;i<sample_length/2;++i) {
		    smp[i] += last;
		    last = smp[i];
		  }
		}
	      else
		{
		  s8 last = 0;
		  s8 *smp = (s8*)sample_data;
		  for(u32 i=0;i<sample_length;++i) {
		    smp[i] += last;
		    last = smp[i];
		  }
		}
	      
	      // Insert sample into the instrument
	      u32 n_samples;
	      if(sample_is_16_bit) {
		n_samples = sample_length/2;
		sample_loop_start /= 2;
		sample_loop_length /= 2;
	      } else {
		n_samples = sample_length;
	      }
	    
	    Sample *sample = new Sample(sample_data, n_samples, 8363, sample_is_16_bit);
	    sample->setVolume(sample_volume);
	    sample->setRelNote(sample_rel_note);
	    sample->setFinetune(sample_finetune); 
	    sample->setLoopStartAndLength(sample_loop_start, sample_loop_length);
	    sample->setLoop(loop_type);
	    sample->setName(sample_name);
	    instrument->addSample(sample);
	    }
	  }
	
	_free(sample_headers);
	
      }
      else
	{
	  // If the instrument has no samples, skip the rest of the instrument header
	  // (which should contain rubbish anyway)
	  file->skip(instinfo->inst_size-29);
	}
      
      _free(instinfo);
    }
  
  //
  // Finish up
  //
  *_song = song;
  
  DC_FlushAll();
  if (file->gotwrong) return 2;
  else return 0;
}

const char *XMTransport::getError(u16 error_id)
{
  return xmtransporterrors[error_id-1];
}

/* ===================== PRIVATE ===================== */
