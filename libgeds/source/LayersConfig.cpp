#include <nds.h>
#include "GuiEngine.h"

const u16 clearchar[] = {
  127, 0,0,0, // up windows
  127, 0,0,0  // down windows
};

const int LAYERS = 4;

void apply();

/* fill our screens with default characters. These are defined in
 *  the Engine::clearchar[] array, which you'll have to override 
 *  in your application if you want different fills.
 * Engine::dontclear can specify which layer should not be erased
 *  in the process. Again, override it in your app to change the
 *  default behaviour.
 */
void default_clearscreen(bool up) {
  u16* vram = up ? WIDGETS_TEXT : SUB_WIDGETS_TEXT;
  u16* chars = up ? (u16*) &clearchar : (u16*) &clearchar + LAYERS;
  u16* layer[] = {vram + Widget::BG, vram, vram + Widget::MAP, vram + Widget::MAP * 2 /* console */};
  for (unsigned j=0;j<LAYERS;j++) {
    u16* what = (u16*) layer[j];
    for (unsigned i=0;i<32*24;i++) 
      what[i]=chars[j];
  }
}

void Window::clearscreen() {
  default_clearscreen(true);
  default_clearscreen(false);
  apply();
}

void apply_gui_layers() {
  // HOOK: change BG_*CNT to match expectations of widgets.
}

void restore_system_colors() {
  u16 *bgpal = Engine::editpal(2, true);
  if (BG_PALETTE_SUB[0xfc]==0) BG_PALETTE_SUB[0xfc]=RGB15(16,0,16);
  if (BG_PALETTE_SUB[0xfd]==0) BG_PALETTE_SUB[0xfd]=RGB15(20,0,20);
  if (bgpal[0xfc]==0) bgpal[0xfc]=RGB15(16,0,16);
  if (bgpal[0xfd]==0) bgpal[0xfd]=RGB15(20,0,20);
  Engine::editpal(2,false);
}

void apply() {
  apply_gui_layers();
  restore_system_colors();
}
