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

#ifndef _NTXM9_H_
#define _NTXM9_H_

#include "song.h"
#include "xm_transport.h"

class NTXM9
{
 public:
  NTXM9();
  ~NTXM9();
  
  // Load the specified xm file from the file system using libfat
  // Returns 0 on success, else an error code
  u16 load(DataReader* file);
  
  // Returns a pointer to a string describing the error corresponding
  // to the given error code.
  const char *getError(u16 error_id);
  
  // Start playing
  void play(bool repeat=true, int potpos=0, int row=0);
  
  // Stop playing
  void stop(void);

  int curpos(); // returns potpos*256+row.
  // probe for errors.
  int getoops(int* p, int *r, int *c);
  void sfx(u8 inst, u8 note, u8 vol, u8 chn);
  void tsfx(u8 pat, u8 row, u8 chan, u8 nch);
 private:
  XMTransport* xm_transport;
  Song *song;
};

#endif
