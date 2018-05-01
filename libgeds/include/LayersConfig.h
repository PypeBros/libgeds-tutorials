#ifndef __LAYERS_CONFIG_H
#define __LAYERS_CONFIG_H

class LayersConfig {
  static const unsigned LAYERS=4;
  static const unsigned SCREENS=2;
public:
  typedef unsigned layersmask_t;
  typedef bool visible_t;
  static const visible_t SHOW = true;
  static const visible_t HIDE = false;
  static const layersmask_t BACK = 1;
  static const layersmask_t TEXT = 2;
  static const layersmask_t YOUR = 4;
  static const layersmask_t CONSOLE = 8;
  static const layersmask_t ALL = 0xf;
  static const u16 default_clearers[];
  LayersConfig(layersmask_t dontclear, const u16 clearers[], bool condown=false);
  /*! clear the content displayed onscreen.
   ** 1) this doesn't hide OAMs.
   ** 2) this won't clear maps that have their corresponding dontclear bit set
   **    (0..3 for main screen, 4..7 for sub-screen. 
   ** 3) Engine::clearchar[i] is used when clearing the _i_th map.
   **/
  virtual void clearscreen();
  virtual void apply();
  virtual void showConsole(visible_t showme);
  virtual ~LayersConfig();
  visible_t consoleShown() { return console_shown; }
protected:
  bool console_shown;
  bool console_down;
  layersmask_t dontclear; //!< set bit I to avoid regular map[I] to be cleared.
  /*<! e.g. use dontclear=LOGS to keep the debug console untouched */

  u16 clearchar[SCREENS*LAYERS];   // what character should be used as "filler" ?
  // clears one of the screens with default attributes, either up or down.
  void default_clearscreen(bool up);
  void apply_gui_layers();
  void restore_system_colors();
};

#endif
