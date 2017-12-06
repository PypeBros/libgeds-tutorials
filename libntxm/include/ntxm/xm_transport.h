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

#ifndef XM_TRANSPORT
#define XM_TRANSPORT

#include "format_transport.h"
#include "datareader.h"
// That should be the amount of free RAM when the program starts
// However, the XM file size and size of the data structure may differ somewhat.
#define MAX_XM_FILESIZE						3738624

#define XM_TRANSPORT_ERROR_INITFAIL				1
#define XM_TRANSPORT_ERROR_FOPENFAIL			2
#define XM_TRANSPORT_ERROR_MAGICNUMBERINVALID		3
#define XM_TRANSPORT_ERROR_MEMFULL				4
#define XM_TRANSPORT_ERROR_PATTERN_READ			5
#define XM_TRANSPORT_FILE_TOO_BIG_FOR_RAM			6
#define XM_TRANSPORT_NULL_INSTRUMENT			7 // Deprecated
#define XM_TRANSPORT_PATTERN_TOO_LONG			8
#define XM_TRANSPORT_FILE_ZERO_BYTE				9

// This class implements loading from and saving to the XM file format
// introduced by Fasttracker II. Man, those were the days!

struct InstInfo {
	u32 inst_size;
	char name[22];
	u8 inst_type;
	u16 n_samples;
	u32 sample_header_size;
	u8 note_samples[96];
	u16 vol_points[24];
	u16 pan_points[24];
	u8 n_vol_points;
	u8 n_pan_points;
	u8 vol_sustain_point;
	u8 vol_loop_start_point;
	u8 vol_loop_end_point;
	u8 pan_sustain_point;
	u8 pan_loop_start_point;
	u8 pan_loop_end_point;
	u8 vol_type;
	u8 pan_type;
	u8 vibrato_type;
	u8 vibrato_sweep;
	u8 vibrato_depth;
	u8 vibrato_rate;
	u16 vol_fadeout;
	u8 reserved_bytes[11];
};

class XMTransport: public FormatTransport {
 public:
  
  // Loads a song from a file and puts it in the song argument
  // returns 0 on success, an error code else
  virtual u16 load(DataReader *file , Song **_song);
  
  const char *getError(u16 error_id);
  virtual ~XMTransport() {};
 private:
};

#endif
