/******************************************************************
 *** SpriteSet management functions for Nintendo DS game engine ***
 ***------------------------------------------------------------***
 *** Copyright Sylvain 'Pype' Martin 2007-2008                  ***
 *** This code is distributed under the terms of the LGPL       ***
 *** license.                                                   ***
 ***------------------------------------------------------------***
 *** last changes:                                              ***
 ***  SpriteSet::getnbtiles required in GameScript::parseline() ***
 ***  SpritePage::changeOAM/setupOAM instead of setOAM required ***
 ***     by GameObject.                                         ***
 *****************************************************************/
#include <nds.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <fat.h>
#include <errno.h>
#include "SpriteSet.h"
#include "SprFile.h"
#include "debug.h"

// attach a map file to a given layer.
bool SpriteSet::LoadMap(const char *filename) {
  // there are tiles loaded already, but we want to replace the map.

  FILE *file = fopen(filename,"rb");
  SprFileTmap hdr;
  if (!file) {
    report("failed to open %s",filename);
    return false;
  }
  step("processing %s",filename);
  fseek(file,0,SEEK_END);
  long at,len=ftell(file);
  fseek(file,0,SEEK_SET);
  u16 *map=0;
  uint width=0, height=0, mlayers=0;
  while ((at=ftell(file))!=len) {
    if (fread(&hdr, sizeof(SprFileUnknown),1,file)!=1) { 
      fclose(file); 
      report("unexpected EOF (%s@%ul)\n",filename,at);
      return false;
    }
    step("@%u '%c%c%c%c' ...", (uint)at, (char)hdr.magic&255,
	 (char)(hdr.magic>>8)&255,(char)(hdr.magic>>16)&255,
	 (char)(hdr.magic>>24)&255);
    switch(hdr.magic) {
    case SPR_FILE_TMAP_MAGIC: {
      if (fread(&(hdr.width),sizeof(u16),2,file)==2) {
	width=hdr.width;
	height=hdr.height;
	mlayers=(hdr.skipsize-2*sizeof(u16)) / (width*height*sizeof(u16));
	step("loading %ix%ix%i map",width,height,mlayers);
	map=(u16*)malloc(width*height*sizeof(u16)*mlayers);
	if (map) {
	  int nb=fread(map,sizeof(u16),width*height*mlayers,file);
	  step("%i/%i cells read.\n",nb,width*height*mlayers);
	  if (nb!=(int)(width*height*mlayers)) 
	    report("oops. wrong size");
	  break; // we're done. quit the loop.
	}
      }
	
      if(map) {
	free(map);map=0;
      }
	
      report("oops. cannot read tile map");
      break; 
    }
    default: 
      step("unknown. skipping %u bytes\n",(uint)hdr.skipsize);
      fseek(file, hdr.skipsize, SEEK_CUR);
    }
  }
  fclose(file);
  if (map)
    return setmap(map,width,height,mlayers);
  return false;
}
