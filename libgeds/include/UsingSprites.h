#ifndef __SPR_VRAM_H
#define __SPR_VRAM_H
#include <sys/types.h>
#include "Resources.h"
/** A dedicated structure for rotations. */
typedef struct gSpriteRotation {
  int16 hdx;
  int16 vdx;
  int16 hdy;
  int16 vdy;
} gSpriteRotation;

//! A bitfield of sprite attribute goodness...ugly to look at but not so bad to use.
typedef union gSpriteEntry
{
  struct {
    struct {
      u16 y			:8;	/**< Sprite Y position. */
      union{
	struct	{
	  u8 			:1;
	  bool isHidden 	:1;	/**< Sprite is hidden (isRotoscale cleared). */
	  u8			:6;
	};
	struct	{
	  bool isRotateScale	:1;	/**< Sprite uses affine parameters if set. */
	  bool isSizeDouble	:1;	/**< Sprite bounds is doubled (isRotoscale set). */
	  ObjBlendMode blendMode:2;	/**< Sprite object mode. */
	  bool isMosaic		:1;	/**< Enables mosaic effect if set. */
	  ObjColMode colorMode	:1;	/**< Sprite color mode. */
	  ObjShape shape	:2;	/**< Sprite shape. */
	};
      };
    };
	  
    union {
      struct {
	u16 x			:9;	/**< Sprite X position. */
	u8 			:7;
      };
      struct {
	u8			:8;
	union {
	  struct {
	    u8			:4;
	    bool hFlip		:1; /**< Flip sprite horizontally (isRotoscale cleared). */
	    bool vFlip		:1; /**< Flip sprite vertically (isRotoscale cleared).*/
	    u8			:2;
	  };
	  struct {
	    u8			:1;
	    u8 rotationIndex	:5; /**< Affine parameter number to use (isRotoscale set). */
	    ObjSize size	:2; /**< Sprite size. */
	  };
	};
      };
    };
    
    struct
    {
      u16 gfxIndex		:10;/**< Upper-left tile index. */
      ObjPriority priority	:2;	/**< Sprite priority. */
      u8 palette		:4;	/**< Sprite palette to use in paletted color modes. */
    };
    
    u16 attribute3;							/* Unused! Four of those are used as a sprite rotation matrice */
  };
  
  struct {
    uint16 attribute[3];
  };

} gSpriteEntry;

class UsingSprites : protected UsingResources {
protected:
  static gSpriteEntry sprites[256]; //0-127 for main screen, 128-255 for sub
//rotation attributes overlap so assign then to the same location
  static gSpriteRotation spriteRotations[64];
 public:
  static void SyncToOam();
  static void DisableAll();
  static void Init();
  static void sanitize();
  static uint snapZlist(bool mainScreen);
  static void restoreZlist(bool mainScreen, uint snapped);
  virtual ~UsingSprites() {}
};

#endif
