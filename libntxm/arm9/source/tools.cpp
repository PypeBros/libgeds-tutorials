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

#include "tools.h"

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <nds.h>

#ifdef ARM9

void* _malloc(size_t size) {
  if (size==0) {
    iprintf("!! SOMEONE REQUESTS 0 BYTES !!\n");
    //    *((size_t*)0xdeadcafe)=size;
    return 0;
  }
  void *p = malloc(size+8);
  if (p) {
    memcpy(p, "pYxM",4);
    memcpy(((char*)p)+size+4, "pYpE",4);
    return (void*)(((char*)p)+4);
  } else {
    iprintf("!! OUT OF MEMORY (less than %i bytes)!!\n",size);
    return 0;
  }
}

void* _calloc(size_t nmemb, size_t size) {
  if (nmemb*size==0) return 0;
  void *p = malloc(nmemb*size+8);
  if (p) {
    memset(p, 0, nmemb*size+8);
    memcpy(p, "pYxM",4);
    memcpy(((char*)p)+nmemb*size+4, "pYpE",4);
    return (void*)(((char*)p)+4);
  } else {
    iprintf("!! OUT OF MEMORY (less than %i bytes)!!\n",nmemb*size);
    return 0;
  }
}


void* _realloc(void *ptr, size_t size) {
  static char magic_chr[]="pYxM";
  static char magic_end[]="pYpE";
  u32* magic=(u32*) magic_chr;
  u32* real=(u32*)ptr;
  real--;
  if (ptr==0) return _malloc(size);
  if (*real!=*magic) {
    iprintf("!! OOPS : %p isn't regular!\n",ptr);
    *((void**)0xdecafbad)=ptr;
    return 0;
  }
  if (size==0) { _free(ptr); return 0; }

  void *p = realloc(real,size+8);
  if (p) {
    memcpy(p, "pYxM",4);
    memcpy(((char*)p)+size+4, "pYpR",4);
    return (void*)(((char*)p)+4);
  } else {
    iprintf("!! OUT OF MEMORY (less than %i bytes)!!\n",size);
    return 0;
  } 
}

void _free(void *ptr) {
  static char magic_chr[]="pYxM";
  u32* magic=(u32*) magic_chr;
  u32* real=(u32*)ptr;
  real--;

  if (!ptr) return; // seems like there are free(NULL) dangling around.

  if (*real != *magic) {
    iprintf("!! OOPS : %p isn't regular!\n",ptr);
    *((void**)0xdecafbad)=ptr;
    return ;
  }
  /* too bad : i cannot check the trailer without knowing buffer size :( */
  *real=~(*real);  // you can't free twice!
  free(real);
}


bool fat_inited = false;

// Helper for converting a string to lowercase
void lowercase(char *str)
{
	for(u8 i=0;i<strlen(str);++i) {
		if((str[i]>=65)&&(str[i]<=90)) {
			str[i]+=32;
		}
	}
}

bool myInitFiles(void)
{
	if(!fat_inited) {
		fat_inited = true;
		return fatInitDefault();
	}
	else
		return true;
}

#endif

void *my_memset(void *s, int c, u32 n)
{
	u8 *t = (u8*)s;
	u32 i;
	for(i=0; i<n; ++i) {
		t[i] = c;
	}
	return s;
}

char *my_strncpy(char *dest, const char *src, u32 n)
{
	u32 i=0;
	while((src[i] != 0) && (i < n)) {
		dest[i] = src[i];
		i++;
	}
	if((i<n)&&(src[i]==0)) {
		dest[i] = 0;
	}
	return dest;
}

#ifdef ARM9

bool fileExists(const char *filename)
{
	myInitFiles();
	
	bool res;
	FILE* f = fopen(filename,"r");
	if(f == NULL) {
		res = false;
	} else {
		fclose(f);
		res = true;
	}
	
	return res;
}

#endif

s32 clamp(s32 val, s32 min, s32 max)
{
	if(val < min)
		return min;
	if(val > max)
		return max;
	return val;
}
