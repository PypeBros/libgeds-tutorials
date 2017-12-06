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

#ifndef _NTXM7_H_
#define _NTXM7_H_

#include "player.h"

class NTXM7
{
	public:
		NTXM7(void (*_playTimerHandler)(void));
		~NTXM7(void);
		
		// override new and delete to avoid linking cruft. (by WinterMute)
		static void* operator new (size_t size);
		static void operator delete (void *p);
		
		void updateCommands(void);
		void timerHandler(void);
		
		void setSong(Song *song);
		void play(int potpos, int row, bool repeat);
		void stop(void);
		void playInst(u8 i, u8 n, u8 v, u8 c);
		void playSfx(u8 p, u8 r, u8 c, u8 n);
	private:
		Player *player;
};

#endif
