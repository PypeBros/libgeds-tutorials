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

#ifndef FORMAT_TRANSPORT_H
#define FORMAT_TRANSPORT_H

#include "song.h"
#include "ntxm/datareader.h"
// This is the abstract base class of transports.
// Transports are classes that handle import and export of songs.
class FormatTransport {
 public:
  
  // Loads a song from a file pots it into _song
  // Returns 0 on success, an error code else
  virtual u16 load(DataReader *file , Song **_song)=0;
  //virtual u16 load(const char *filename, Song **_song) = 0;
  
  virtual ~FormatTransport() {};
  
 private:
};

#endif
