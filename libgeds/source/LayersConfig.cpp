#include <nds.h>
#include "GuiEngine.h"
#include "LayersConfig.h"

/* fill our screens with default characters. These are defined in
 *  the Engine::clearchar[] array, which you'll have to override 
 *  in your application if you want different fills.
 * Engine::dontclear can specify which layer should not be erased
 *  in the process. Again, override it in your app to change the
 *  default behaviour.
 */
void LayersConfig::default_clearscreen(bool up) {
  u16* vram = up ? WIDGETS_TEXT : SUB_WIDGETS_TEXT;
  layersmask_t clearmask = up ? dontclear : (dontclear >> LAYERS);
  u16* chars = up ? (u16*) &clearchar : (u16*) &clearchar + LAYERS;
  u16* layer[] = {vram + Widget::BG, vram, vram + Widget::MAP, vram + Widget::MAP * 2 /* console */};
  for (unsigned j=0;j<LAYERS;j++) {
    u16* what = (u16*) layer[j];
    if (clearmask & (1<<j)) continue; 
    for (unsigned i=0;i<32*24;i++) 
      what[i]=chars[j];
  }
}

void LayersConfig::clearscreen() {
  default_clearscreen(true);
  default_clearscreen(false);
  apply();
}

void LayersConfig::showConsole(visible_t visible) {
  const u16 common = BG_MAP_BASE(GECONSOLE)|BG_TILE_BASE(0);
  if (console_down) {
    REG_BG3CNT_SUB = common|BG_PRIORITY(visible?0:3);
    if (visible)
      REG_DISPCNT_SUB |= DISPLAY_BG3_ACTIVE;
  } else {
    REG_BG0CNT = common|BG_PRIORITY(visible?0:3);
    if (visible)
      REG_DISPCNT |= DISPLAY_BG0_ACTIVE;
  }
  console_shown = visible;
}

LayersConfig::LayersConfig(layersmask_t mask, const u16 chars[], bool condown) :
  console_shown(SHOW), console_down(condown),
  dontclear(mask), clearchar()
{
  memcpy(clearchar, chars, sizeof(clearchar));
}

LayersConfig::~LayersConfig()
{
}

void LayersConfig::apply_gui_layers() {
  REG_BG1CNT = BG_MAP_BASE(GEWIDGETS)|BG_TILE_BASE(0)|BG_PRIORITY(1); 
  REG_BG2CNT = BG_MAP_BASE(GEBGROUND)|BG_TILE_BASE(0)|BG_PRIORITY(2)|BG_COLOR_256;
  REG_BG1HOFS=0;
  REG_BG1VOFS=0;
  REG_BG2HOFS=0;
  REG_BG2VOFS=0;
  REG_DISPCNT &= ~(DISPLAY_BG0_ACTIVE|DISPLAY_BG3_ACTIVE|ENABLE_3D);
  REG_DISPCNT |= DISPLAY_BG1_ACTIVE | DISPLAY_BG2_ACTIVE;
  REG_BLDCNT = 0;
  
  REG_BG0HOFS_SUB=0;
  REG_BG0VOFS_SUB=0;
  REG_BG1HOFS_SUB=0;
  REG_BG1VOFS_SUB=0;
  REG_BG2HOFS_SUB=0;
  REG_BG2VOFS_SUB=0;
  REG_BG0CNT_SUB = BG_MAP_BASE(GEBGROUND)|BG_TILE_BASE(0)|BG_PRIORITY(2)|BG_COLOR_256;
  // sub_bg1 is for the grid itself, tool boxes, etc. (alpha-blended)
  REG_BG1CNT_SUB = BG_MAP_BASE(GEWIDGETS)|BG_TILE_BASE(0)|BG_PRIORITY(1);
  REG_DISPCNT_SUB |= DISPLAY_BG1_ACTIVE | DISPLAY_BG2_ACTIVE;

}


void LayersConfig::restore_system_colors() {
  u16 *bgpal = Engine::editpal(2, true);
  if (BG_PALETTE_SUB[0xfc]==0) BG_PALETTE_SUB[0xfc]=RGB15(16,0,16);
  if (BG_PALETTE_SUB[0xfd]==0) BG_PALETTE_SUB[0xfd]=RGB15(20,0,20);
  if (bgpal[0xfc]==0) bgpal[0xfc]=RGB15(16,0,16);
  if (bgpal[0xfd]==0) bgpal[0xfd]=RGB15(20,0,20);
  Engine::editpal(2,false);
}

void LayersConfig::apply() {
  apply_gui_layers();
  restore_system_colors();
  REG_BG3CNT = BG_MAP_BASE(GEYOURMAP)|BG_TILE_BASE(1)|BG_COLOR_256|BG_PRIORITY(1);

  // this is for drawing the background.
  // it also contains the palette, so that we can "scale" grid & grid data if needed.
  REG_BG2CNT_SUB = BG_MAP_BASE(GEYOURMAP)|BG_PRIORITY(3);
  if (!console_down) 
    REG_BG3CNT_SUB = BG_MAP_BASE(GEYOURMAP)|BG_PRIORITY(3);

  showConsole(console_shown);
  
  // ensure scrolling from previous app do not interfere here.
  REG_BG3HOFS_SUB=0;
  REG_BG3VOFS_SUB=0;

  /* SUB_BG3_XDX */ REG_BG3PA_SUB = 1 << 8;
  /* SUB_BG3_XDY */ REG_BG3PB_SUB = 0;
  /* SUB_BG3_YDX */ REG_BG3PC_SUB = 0;
  /* SUB_BG3_YDY */ REG_BG3PD_SUB = 1 << 8;
  //our bitmap looks a bit better if we center it so scroll down (256 - 192) / 2
  /* SUB_BG3_CX */ REG_BG3X_SUB = 0;
  /* SUB_BG3_CY */ REG_BG3Y_SUB = 0;

}
