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

#include "ntxm/ntxm7.h"

#include <stdlib.h>

#include "command.h"

void* NTXM7::operator new (size_t size) {
 
	return malloc(size);
 
} // default ctor implicitly called here

void NTXM7::operator delete (void *p) {
 
	if ( NULL != p ) free(p);
 
} // default dtor implicitly called here

NTXM7::NTXM7(void (*_playTimerHandler)(void))
	:player(0)
{
	player = new Player(_playTimerHandler);
}

NTXM7::~NTXM7(void)
{
	delete player;
}

// void NTXM7::updateCommands(void) // This has been replaced by the 
//{                                 // CommandRecvHandler that is set as datamsgHandler
//	CommandProcessCommands();   // in CommandInit.
//}

void NTXM7::timerHandler(void)
{
	player->playTimerHandler();
}

void NTXM7::setSong(Song *song)
{
	player->setSong(song);
}

void NTXM7::play(int potpos, int row, bool repeat)
{
	player->play(potpos, row, repeat);
}

void NTXM7::stop(void)
{
	player->stop();
}

void NTXM7::playInst(u8 ins, u8 note, u8 vol, u8 chn) {
  player->playInstrument(ins, note, vol, chn);
}

void NTXM7::playSfx(u8 pot, u8 row, u8 chn, u8 nchs) {
  player->playSfx(pot, row, chn, nchs);
}
