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

#include "demokit.h"

#define timers2ms(tlow,thigh)(tlow | (thigh<<16)) >> 5

int ticksSpeed;
unsigned int lastTime;
unsigned int timeCounted;
int clockStopped;

void demoInit(void)
{
	reStartRealTicks();
	reStartTicks();
}

void reStartRealTicks(void)
{
	TIMER2_DATA=0;
	TIMER3_DATA=0;
	TIMER2_CR=TIMER_DIV_1024 | TIMER_ENABLE;
	TIMER3_CR=TIMER_CASCADE | TIMER_ENABLE;
}

unsigned int getRealTicks(void)
{
	return timers2ms(TIMER2_DATA, TIMER3_DATA);
}

void reStartTicks(void)
{
	ticksSpeed = 100;
	clockStopped = 0;
	setTicksTo(0);
}

void startTicks(void)
{
	if (clockStopped) {
		clockStopped = 0;
		lastTime = getRealTicks();
	}
}

void stopTicks(void)
{
	if (!clockStopped) {
		clockStopped = 1;
		getTicks();
	}
}

void setTicksTo(unsigned int time)
{
	timeCounted = time;
	lastTime = getRealTicks();
}

unsigned int getTicks(void)
{
	unsigned int t = ((getRealTicks() - lastTime)*ticksSpeed)/100;
	if ((t > 0) || (-t < timeCounted)) {
		timeCounted += t;
	} else {
		timeCounted = 0;
	}
	lastTime = getRealTicks();
	return timeCounted;
}

void setTicksSpeed(int percentage)
{
	getTicks();
	ticksSpeed = percentage;
}

int getTicksSpeed(void)
{
	return ticksSpeed;
}

void delay(unsigned int d) {
	unsigned int start = getTicks();
	while (getTicks() <= start+d);
}


int my_rand(void)
{
	static int seed = 2701;
	return seed = ((seed * 1103515245) + 12345) & 0x7fffffff;
}

