#include <nds.h>
#include "UsingSprites.h"
#include <stdio.h>

gSpriteEntry UsingSprites::sprites[256];

void UsingSprites::DisableAll() {
  gResources->reset(RES_OAM);
  gResources->reset(RES_OAM_SUB);

  for (unsigned i=0;i<256;i++) 
    sprites[i].attribute[0]=ATTR0_DISABLED;

  // make sure we flushed the cache for OAM copy 
  DC_FlushRange(sprites,256*sizeof(SpriteEntry));

  // update all registered sprites.
  dmaCopy(sprites, OAM, 
	  128 * sizeof(SpriteEntry));
  dmaCopy(sprites+128, OAM_SUB, 
	  128 * sizeof(SpriteEntry));
}

void UsingSprites::Init() {
    for(uint i = 0; i < 128; i++) {
    sprites[i].attribute[0] = ATTR0_DISABLED;
    sprites[i].attribute[1] = 0;
    sprites[i].attribute[2] = 0;
  }
}


/** not-yet-allocated OAMs may have weird state. Make sure they're all reset
 * to 'disabled' and won't clutter the game look.
 */
void UsingSprites::sanitize() {
  unsigned maincount = gResources->get(RES_OAM);
  unsigned subcount = gResources->get(RES_OAM_SUB);
  for (unsigned i=maincount;i<128;i++) 
    sprites[i].attribute[0]=ATTR0_DISABLED;
  for (unsigned i=subcount;i<256;i++) 
    sprites[i].attribute[0]=ATTR0_DISABLED;

  // make sure we flushed the cache for OAM copy 
  DC_FlushRange(sprites,256*sizeof(SpriteEntry));

  // update all registered sprites.
  dmaCopy(sprites, OAM, 
	  128 * sizeof(SpriteEntry));
  dmaCopy(sprites+128, OAM_SUB, 
	  128 * sizeof(SpriteEntry));
}

void UsingSprites::SyncToOam()
{
  unsigned n=gResources->get(RES_OAM), nsub=gResources->get(RES_OAM_SUB);

  u16* p = (u16*) OAM;
  for (uint i=0;i<n;i++) {
    *p++=sprites[i].attribute[0];
    *p++=sprites[i].attribute[1];
    *p++=sprites[i].attribute[2];
    p++; // skip rotations for now.
  }

  // update all registered sprites.
  p = (u16*) OAM_SUB;
  for (uint i=128;i<128+nsub;i++) {
    *p++=sprites[i].attribute[0];
    *p++=sprites[i].attribute[1];
    *p++=sprites[i].attribute[2];
    p++;
  }
}
