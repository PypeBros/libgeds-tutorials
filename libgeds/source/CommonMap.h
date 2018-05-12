#ifndef __COMMON_MAP_H
#define __COMMON_MAP_H
#include "interfaces.h"

/** This is what the GameScript sees from scrolling playable layers.
 * how it actually scrolls is an implementation detail for the sub-classes.
 */
class CommonMap : public iWorld {

  NOCOPY(CommonMap);
protected:
  typedef int spx; //!< 'signed pixels'
  typedef int stl; //!< 'signed tiles'
  typedef uint upx; //!< 'unsigned pixels
  typedef uint utl; //!< 'unsigned tiles

  u16* data;   //!< the map data, unpacked, line per line
  world_tile_t tw, th;  //!< map width and height (in tiles)
  u16 xscreen, yscreen; //!< caching scroll registers until VBL.
  bool owndata;

  vu16* getScrollingRegister(unsigned physlayer);

  virtual tile_properties_t getflags(world_tile_t tx, world_tile_t ty, GameObject *who);
public:
  /** tries to grab the map in the spriteset if present.
   **   map data is never owned in this case.
   **/
  CommonMap(SpriteSet* set, unsigned physlayer, unsigned plane=0);
  

  /** automatically allocates a w x h buffer and own it:
   *  congrats, you've created an empty map.
   */
  CommonMap(world_tile_t w, world_tile_t h, unsigned physlayer);

  virtual ~CommonMap();
  
  world_pixel_t getpxwidth() {return tw*8;}
  world_pixel_t getpxheight(){return th*8;}

  virtual void lookAt(world_pixel_t px, world_pixel_t py) = 0;
  
  //! scroll to the new coordinates.
  /** this can be applied at any time, as only off-screen tiles are updated. */
  virtual void scrollTo(world_pixel_t px, world_pixel_t py) = 0;

  static 
  CommonMap* CreateSimpleMap(SpriteSet *set, u16* screen,
			     unsigned physlayer, unsigned plane=0);
  static 
  CommonMap* CreateSimpleMap(world_tile_t w, world_tile_t h, u16* screen, unsigned physlayer);

  static 
  CommonMap* CreateInfiniMap(SpriteSet *set, u16* screen,
			     unsigned physlayer, unsigned plane=0);

  static 
  CommonMap* CreateInfiniMap(world_tile_t w, world_tile_t h, u16* screen, unsigned physlayer);

  virtual bool needsVertical()=0;
};
#endif
