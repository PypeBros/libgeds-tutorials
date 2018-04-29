#include <nds.h>
#include "GuiEngine.h"
#include "SpriteSet.h"
#include "interfaces.h"
#include "CommonMap.h"

CommonMap::CommonMap(SpriteSet* set, unsigned physlayer, unsigned plane) :
    data(0),tw(0),th(0),
    xscreen(0),yscreen(0),
    owndata(false)
  {
    data=set->getmap(&tw,&th);
    if (plane) data+=plane*tw*th;
  }

CommonMap::CommonMap(world_tile_t w, world_tile_t h, unsigned physlayer) :
    data(0),tw(w),th(h),
    xscreen(0),yscreen(0),
    owndata(true)
  {
    data=(u16*)malloc(w*h*sizeof(u16));
    iprintf("new map created @%p",data);
    memset(data,0,w*h*sizeof(u16));
  }

CommonMap::~CommonMap() {
    if (owndata && data) free(data);
  }

/** Implementation of level layer that fits the VRAM completely.
 *  that would be 32x64 tiles or 64x32 tiles.
 */
class SimpleMap : public CommonMap {
  NOCOPY(SimpleMap);
  u16 *onscreen; //!< two 32x32 tiles block on the screen.
  world_pixel_t xadjust, yadjust;
public:

  void lookAt(world_pixel_t px, world_pixel_t py) {
    xadjust = px;
    yadjust = py;
    if (tw > th) {
      for (int i=0; i<th; i++) {
	/* map file has line of mapwidth tiles, screen is two consecutive chunks
	 *  of 32x32 data. A single dmaCopy can't do it. */
	dmaCopy(data + i*tw, (onscreen) + i*32, 64);
	dmaCopy(data+32 + i*tw, (onscreen)+ i*32 + 1024, 64);
      }
    } else {
      for (int i=0; i<th; i++) {
	dmaCopy(data + i*tw, (onscreen) + i*32, 64);
      }
    }
  }

  void scrollTo(world_pixel_t px, world_pixel_t py) {
  }

  SimpleMap(SpriteSet* set, u16* screen, unsigned physlayer, unsigned plane=0) :
    CommonMap(set, physlayer, plane),
    onscreen(screen),
    xadjust(0), yadjust(0)
  {
  }

  
  SimpleMap(world_tile_t w, world_tile_t h, u16 *screen, unsigned physlayer) :
    CommonMap(w, h, physlayer),
    onscreen(screen),
    xadjust(0), yadjust(0)
  {
  }

  bool needsVertical() {
    return tw < th;
  }
};

CommonMap* CommonMap::CreateSimpleMap(SpriteSet *set, u16* screen,
				      unsigned physlayer, unsigned plane) {
  return new SimpleMap(set, screen, physlayer, plane);
}

CommonMap* CommonMap::CreateSimpleMap(world_tile_t w, world_tile_t h, u16* screen, unsigned physlayer) {
  return new SimpleMap(w, h, screen, physlayer);
}

CommonMap* CommonMap::CreateInfiniMap(SpriteSet *set, u16* screen,
				      unsigned physlayer, unsigned plane) {
  return 0; // HOOK
}
  
CommonMap* CommonMap::CreateInfiniMap(world_tile_t w, world_tile_t h, u16* screen, unsigned physlayer) {
  return 0; // HOOK
}
