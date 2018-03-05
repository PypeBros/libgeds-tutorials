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
#include <nds/arm9/math.h>
#include <nds/arm9/console.h>
#include <nds/arm9/trig_lut.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <fat.h>
#include <errno.h>
#include "SpriteSet.h"
#include "debug.h"


bool SpritePage::setOAM(blockno_t sprid, gSpriteEntry *oam) const {

  static u16 shape[5][5]={
    {0,0,0,0,0}, // 0 x *
    {0,ATTR0_SQUARE,ATTR0_WIDE,0,0}, // only 1x1 and 1x2
    {0,ATTR0_TALL,ATTR0_SQUARE,ATTR0_WIDE,ATTR0_WIDE}, // 2x1,2x2,2x3,2x4
    {0,0,ATTR0_WIDE,ATTR0_SQUARE,ATTR0_SQUARE}, // 3x2, 3x3, 3x4
    {0,0,ATTR0_TALL,ATTR0_SQUARE,ATTR0_SQUARE}, // 4x2, 4x3, 4x4
  };

  static u16 size[]={
    0,ATTR1_SIZE_8,ATTR1_SIZE_8, 0, //  --,  1x1, 2x1/1x2, --
    ATTR1_SIZE_16,0,ATTR1_SIZE_16,0, // 2x2, --,  3x2/2x3, --
    ATTR1_SIZE_16,ATTR1_SIZE_32,0,0, // 2x4/4x2, 3x3, --,  --
    ATTR1_SIZE_32,0,0,0,             // 3x4/4x3
    ATTR1_SIZE_32                    // 4x4
  };
  if (sprid == INVALID_BLOCKNO) return false;
  oam->attribute[0]&=0xff; // just keep Y...
  oam->attribute[0]|=shape[height][width]|ATTR0_COLOR_256|ATTR0_NORMAL;
  oam->attribute[1]&=0x1ff;// here keep X...
  oam->attribute[1]|=size[width*height];
  oam->attribute[2]=((*this)[sprid])/granularity_divisor;
  return true;
}


/* okay, now i want to see sprite ID xxxx displayed by OAM yyyy */
/* for now, just assume that sprid xxxx uses tiles xxxx*4 .. 3+xxxx*4 */
bool SpritePage::setupOAM(gSpriteEntry *oam, u16 flags) const {

  static u16 shape[5][5]={
    {0,0,0,0,0}, // 0 x *
    {0,ATTR0_SQUARE,ATTR0_WIDE,0,0}, // only 1x1 and 1x2
    {0,ATTR0_TALL,ATTR0_SQUARE,ATTR0_WIDE,ATTR0_WIDE}, // 2x1,2x2,2x3,2x4
    {0,0,ATTR0_WIDE,ATTR0_SQUARE,ATTR0_SQUARE}, // 3x2, 3x3, 3x4
    {0,0,ATTR0_TALL,ATTR0_SQUARE,ATTR0_SQUARE}, // 4x2, 4x3, 4x4
  };

  static u16 size[]={
    0,ATTR1_SIZE_8,ATTR1_SIZE_8, 0, //  --,  1x1, 2x1/1x2, --
    ATTR1_SIZE_16,0,ATTR1_SIZE_16,0, // 2x2, --,  3x2/2x3, --
    ATTR1_SIZE_16,ATTR1_SIZE_32,0,0, // 2x4/4x2, 3x3, --,  --
    ATTR1_SIZE_32,0,0,0,             // 3x4/4x3
    ATTR1_SIZE_32                    // 4x4
  };

  oam->attribute[0]=shape[height][width]|ATTR0_COLOR_256|ATTR0_DISABLED|flags;
  oam->attribute[1]=size[width*height];
  oam->attribute[2]=0;
  return true;
}

void SpritePage::changeOAM(gSpriteEntry *oam, u16 x, u16 y, blockno_t sprid) const {
  oam->attribute[0]=(oam->attribute[0]&0xfc00)|(y&0x00ff); // ATTR0_NORMAL rest
  oam->attribute[1]=(oam->attribute[1]&0xfe00)|(x&0x01ff);
  oam->attribute[2]=(oam->attribute[2]&0xf000)|(*this)[sprid];

}
