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

#ifndef _TOOLS_H_
#define _TOOLS_H_

#include <stdlib.h>
#include <stdio.h>
#include <fat.h>

void lowercase(char *str);
void *my_memset(void *s, int c, u32 n);
char *my_strncpy(char *dest, const char *src, u32 n);
bool myInitFiles(void);
bool fileExists(const char *name);
s32 clamp(s32 val, s32 min, s32 max);

void *_calloc(size_t nmemb, size_t size);
void *_malloc(size_t size);
void _free(void *ptr);
void *_realloc(void *ptr, size_t size);


#endif
