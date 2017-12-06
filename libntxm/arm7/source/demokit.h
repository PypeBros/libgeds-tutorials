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

#ifndef _DEMOKIT_H_
#define _DEMOKIT_H_

#include <nds.h>

void demoInit(void);
void reStartRealTicks(void);
unsigned int getRealTicks(void);

void reStartTicks(void);
void startTicks(void);
void stopTicks(void);
void setTicksTo(unsigned int time);
unsigned int getTicks(void);
void setTicksSpeed(int percentage);
int getTicksSpeed(void);
void delay(unsigned int d);

int my_rand(void);

#endif
