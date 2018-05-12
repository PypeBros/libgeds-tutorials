#include <nds.h>
#include <interfaces.h>
enum { f_plyfallthru, f_plyjumpthru, f_plyleftthru, f_plyrightthru,
       f_monsterthru, };

tile_properties_t GameObject::raiser_flag = F_NORAISER;
tile_properties_t iWorld::properties[]={
  F_NORAISER|F_PLAYERTHRU|F_FALLTHRU|F_MONSTERTHRU, // EMPTY, 
  F_NORAISER|F_WATER|F_PLAYERTHRU,       // water
  F_NORAISER|F_BLOCKING|F_FLOOR,         // block
  F_NORAISER|F_FLOOR|F_PLAYERTHRU|F_MONSTERTHRU,       // just floor

  F_NORAISER|F_PLAYERTHRU|F_FALLTHRU|F_MONSTERTHRU|F_RSLOPE|F_FLOOR,                    // / slope
  F_NORAISER|F_PLAYERTHRU|F_FALLTHRU|F_MONSTERTHRU|F_LSLOPE|F_FLOOR,                    // \ slope
  F_NORAISER|F_FLOOR,                    // undef
  F_NORAISER|F_FLOOR,                    // undef

  F_NORAISER|F_FLOOR,                    // undef
  F_NORAISER|F_FLOOR,                    // undef
  F_NORAISER|F_CLIMB|F_PLAYERTHRU|F_FALLTHRU|F_MONSTERTHRU,  // rope
  F_PLAYERTHRU|F_MONSTERTHRU|F_FALLTHRU, // "shadow" (raises the GOBs).
  F_PLAYERTHRU|F_FALLTHRU|F_MONSTERTHRU, // extra xx00yyzz (mario bonus)
  F_PLAYERTHRU|F_FALLTHRU,               // extra xx01yyzz (qwak bonus)
  0,                                     // extra xx10yyzz (doors)
  F_PLAYERTHRU,                          // extra xx11yyzz (consumable platforms, Keen 2)
};
