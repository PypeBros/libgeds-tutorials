/* -*- c++ -*- */
#ifndef __GUI_ENGINE_H
#define __GUI_ENGINE_H

#include "nocopy.h"

/**********************************************************************************
 ** The new VRAM layout : 16KB under the control of the GuiEngine.               **
 **   addr    :   content                                      : block : charset **
 ** ----------:------------------------------------------------:-----------------**
 ** 0000-0fff : standard ASCII charset (128 car.), 4bpp        : 0-1   :    0    **
 ** 1000-1fff : extra char (available through print functions) : 2-3   :         **
 ** 2000-27ff : bg map                                         : 4     :         **
 ** 2800-2fff : overlay/console map                            : 5     :         **
 ** 3000-37ff : your app's custom maps                         : 6     :         **
 ** 3800-3fff : debug console / custom map (depends on screen) : 7     :         **
 ** 4000-ffff : 48KB of VRAM free for custom tiles, maps, etc. : 8-31  :   1-4   **
 **           : the upper 64-128KB is completely up to you :)  : ####  :         **
 **********************************************************************************/
 
extern "C" {
#include <nds.h>
}
#define SUB_WIDGETS_CHARSET(x) (BG_GFX_SUB+(x)*16)
#define WIDGETS_CHARSET(x) (BG_GFX+(x)*16)
#define SUB_YOUR_CHARSET(x) (BG_GFX_SUB+(0x1000/sizeof(u16))+(x)*16)
#define YOUR_CHARSET(x) (BG_GFX+(0x1000/sizeof(u16))+(x)*16)

#define GEBGROUND 4
#define GEWIDGETS 5
#define GEYOURMAP 6
#define GEYOURALT 7
#define GECONSOLE 7

#define SUB_WIDGETS_CONSOLE ((u16*)SCREEN_BASE_BLOCK_SUB(GECONSOLE)) // excludes MAP.
#define SUB_WIDGETS_MAP ((u16*)SCREEN_BASE_BLOCK_SUB(GEYOURALT)) //BG3
#define SUB_WIDGETS_TEXT ((u16*)SCREEN_BASE_BLOCK_SUB(GEWIDGETS)) //BG1
#define SUB_WIDGETS_BACKGROUND ((u16*)SCREEN_BASE_BLOCK_SUB(GEBGROUND)) 

#define WIDGETS_BACKGROUND ((u16*)SCREEN_BASE_BLOCK(GEBGROUND)) 
#define WIDGETS_MAP  ((u16*)SCREEN_BASE_BLOCK(GEYOURMAP))       
#define WIDGETS_TEXT ((u16*)SCREEN_BASE_BLOCK(GEWIDGETS))    
#define WIDGETS_CONSOLE ((u16*)SCREEN_BASE_BLOCK(GECONSOLE))


#include <vector>
#include <stdio.h>

typedef unsigned oam_t; // a slot in the Object Attributes Memory
typedef uint16_t screen_pixel_t; // coordinate in pixels with 0 being screen corner
typedef uint world_pixel_t;      // coordinate in pixels related to level corner
typedef uint widget_pixel_t;
typedef uint screen_tile_t;
typedef int world_tile_t;
typedef screen_pixel_t sp_t;
typedef world_pixel_t wp_t;
typedef screen_tile_t st_t;
typedef world_tile_t wt_t;

#define KEY_DPAD (KEY_RIGHT|KEY_LEFT|KEY_UP|KEY_DOWN)
#define KEY_ABXY (KEY_A|KEY_B|KEY_X|KEY_Y)

enum BgTransform {
  BGx_XDX=0, BGx_XDY=2,BGx_YDX=4,BGx_YDY=6,BGx_CX=8
};

#define BG2s16(x) (*(vs16*)(0x4000020+x))
#define BG3s16(x) (*(vs16*)(0x4000030+x))
#define BG2s16_SUB(x) (*(vs16*)(0x4001020+x))
#define BG3s16_SUB(x) (*(vs16*)(0x4001030+x))

enum Event {
  GUI_NOPE=0,     //!< nothing occured :(
  GUI_TOUCHED =1, //!< just touched ... 
  GUI_RELEASED=2, //!< now no longer touched
  GUI_CLICKED =4, //!< clicked-then-released for those who
  GUI_MOVED   =8, //!< still being touched.
  GUI_DIRECTION=16,  //!< for Directional arrows
  GUI_BUTTON   = 32, //!< for ABXY
  GUI_SPECIAL  = 64, //!< START, SELECT, L, R, close DS.
  GUI_ALT_TOUCHED = 128,
  GUI_ALT_CLICKED = 256, //!< clicked while pressing L
  GUI_STYLUS  =384|14,//!< any stylus-related event but clicks.
  GUI_ANYKEY = 0x200,  //!< only if transient == true.
  GUI_WINCOM = 0x400,  //!< win2win communications.
  END_OF_LINE = 0x4000,
  HOP_TO_ZERO = 0x8000 //!< 
};

enum GuiConfig {
  NORMAL,
  MORE_BG_MEMORY,  //!< 256K on BG_GFX, but reduces SUB_SPRITE to 16K
  MORE_SPR_MEMORY, //!< 256K on SPR_GFX, "    "      "   "      " " 
  BG256_SPR64_MEMORY, //!< don't touch on SUB_xx. uses E for sprites.
  MEMORY_SETTINGS=3,  //!< mask for all *_MEMORY options.
  EXTENDED_PALETTES=4, //!< use DISPLAY_BG_EXT_PALETTE
  CONSOLE_DOWN=8, //!< DownWindows (SUB screen) compete with Console
  EXTENDED_SPRPALS=16, //!< use DISPLAY_SPR_EXT_PALETTE
  SWAP_COLORS_ON_L=32,
};

#include "debug.h"

class Nowidget; // internal use for Engine.cpp

class Widget {
  NOCOPY(Widget);
public:
  static const int BDEL=124;
  static const int BON=126;
  static const int BOFF=125;
  static const int BCHECK=127;
  static const int BG=-0x400;
  static const int MAP=0x400;
  Widget(int internalonly):width(0),height(0),vram(0) {};
  friend class Nowidget; // internal use.
protected:
  //! tells whether class initialization should be called.
  static bool prepared;
  static void prepare(void);
  u16 width,height;
  u16* vram; // where to redraw;

public:
  //! build a widget at a given position
  /** make sure you have your own prepared boolean and call
   **  prepare() function of your class in your derivatives
   **  constructors.
   **/
  Widget() : width(1),height(1),vram(0) {
    if (!prepared) Widget::prepare();
  }

  virtual void place(u16* vr, struct WidgCoords &wc)=0;
  virtual void render(void)=0;

  virtual ~Widget();

  /*! clicks handler ... you _must_ overload this */
  virtual bool handle(widget_pixel_t x, widget_pixel_t y, Event evt)=0;
  /*! keypad handler. ignore stuff by default */
  virtual bool handle(uint& pressed, Event evt) { 
    (void) pressed; (void) evt;
    return false;
  }
};

class WidgCoords {
  //! pixel coordinates of the widget.
  screen_pixel_t xbase, ybase;
  //! pixel dimensions of the widget
  screen_pixel_t width, height;
  uint16 mask; //!< built of Events that we allow.
  Widget* wptr;
public:
  WidgCoords(class Widget* w,screen_pixel_t x, screen_pixel_t y)
    : xbase(x), ybase(y),
      width(0),height(0),mask(0),wptr(w)
  {
    if (!w) die(__FILE__, __LINE__);
  }

  inline bool samewidget(Widget* w) {
    return wptr==w;
  }

  void getcoords(screen_pixel_t* x, screen_pixel_t* y) {
    *x=xbase;
    *y=ybase;
  }

  void setup(screen_pixel_t w, screen_pixel_t h=8, u16 m=GUI_TOUCHED) {
    width=w; height=h; mask=m;
  }
  uint16 getMask(void) {
    return mask;
  }

  //! tells whether a given position falls in the widget area.
  /*! you should only overload this method if your widget's area
   *  is not rectangular. Otherwise, what you want to change is
   *  probably handle()
   */
  bool test(touchPosition touch) {
    screen_pixel_t x=touch.px, y=touch.py;
    if (x>=xbase && y>=ybase && x<xbase+width && y<ybase+height)
      return true;
    else return false;
  }

  bool last() {
    return (mask&END_OF_LINE)!=0;
  }

  bool chain() {
    return (mask&HOP_TO_ZERO)!=0;
  }

  inline void render(void) { wptr->render(); }
  inline bool handle(touchPosition touch, Event evt) {
    return wptr->handle(touch.px-xbase,touch.py-ybase,evt);
  }
  inline bool handle(uint& pressed, Event evt) {
    if (!this) die(__FILE__, __LINE__);
    if (!wptr) die(__FILE__, __LINE__);
    return wptr->handle(pressed,evt);
  }
};

class LayersConfig;

class Window {
  NOCOPY(Window);
  //! default configuration for using the GUI.
  static LayersConfig* GuiLayers;
 protected:
  uint16 nbWidgets,wstart;
  Window* super;
  LayersConfig* layers;
  std::vector<struct WidgCoords> obj;
 public:
  bool transient; //!< a transient window will recieve GUI_ANYKEY to handle
  bool isdown;
  bool active;    //!< an active window is checked for widgets selection.
  bool swap_colors;
  /* recursively test whether this is a sub-window of W */
  bool issubwin(Window *w) {
    if (!super) return false;
    if (super==w) return true;
    else return super->issubwin(w);
  }
  unsigned setmark(uint mode=END_OF_LINE);

  virtual Widget* add(screen_pixel_t x, screen_pixel_t y, Widget* w)=0;
  /* receive a "help page" and returns the id of the next page */
  virtual int help(int no);
  Window(LayersConfig *lc = 0) :
    nbWidgets(0),wstart(0),super(0),layers(lc),obj(),
    transient(false),isdown(true),active(true), swap_colors(true) {
  }
  Window* holder() { return super;}

  /* draw the whole window on screen */
  virtual void render(void);
  virtual void clearscreen(void);
  bool activate(bool yes) {
    bool last=active;
    active=yes;
    return last;
  }
  /* return the widget that contains the touch position */
  WidgCoords* test(touchPosition touch);

  WidgCoords* widget(Widget* w);

  /* handle keypresses that the current widget doesn't accept */
  virtual bool handle(uint& pressed, Event evt) { 
    (void) pressed; (void) evt; return false;
  }

  /* overload if you want to do some state cleaning when the engine
     is about to switch to another window ... */
  virtual void release(void) {}

  /* overload if you want to do some state restore when the engine
     switch back to our window. */
  virtual void restore(void) {}

  virtual ~Window() {}
  void dump();
};

class iButtonListener {
 public:
  virtual bool event(Widget* w, uint code)=0;
  virtual ~iButtonListener() {};
};

class Engine {
  NOCOPY(Engine);
#ifdef GLOBAL_DEBUGGER
  friend GLOBAL_DEBUGGER;
#endif
  Window* interface;
  Window* nextWindow;  // for deferring window switch.
  WidgCoords* current;
  touchPosition lastpos;
  uint16 lastbuttons;
  uint32 magic;
  static uint held;

  static Engine* singleton;
 public:
  static bool prepared() { return singleton!=0; }
  bool lefthand;
 private:
  enum GuiConfig config;
  bool swap_colors;
  int vcno; // which vcount has been processed last ?
  //  Animator* ClearAnimatorList(Animator* l);
 public:
  inline static bool holding(uint lr) {
    return (held & lr)==lr;
  }
  Engine(enum GuiConfig=NORMAL, Window* itf=0, u16 bgcolor=0);
#ifdef DYNAMIC_ENGINE
  ~Engine();
#endif
  static bool isConsoleDown() { return singleton->config & CONSOLE_DOWN; }
  void setWindow(Window* itf); /* use changeWindow if you need a static one */
  static uint getKeysHeld() { return singleton->held;}
  static void enable3D();
  static void DebugConsole(bool showit);
  // static void ClearAnimators();
  Window *curwin() { return interface; }
  void handle(void);
  void sync(void);
  void prepare(void);
  void animate(void);
  void release(void) { current=0; } // shouldn't be used except with dyndows
  bool focus(Widget* w);
  void showSprites(bool show);
  static void sanitize();
  static bool selected(Widget* w) {
    if (!singleton->current) return false;
    return singleton->current->samewidget(w);
  }

  /** enable/disable edition as needed; 
   *   return the pointer towards the effective (default) palette for
   *   a given background.
   */
  static u16* editpal(uint bg, bool edit);

  static void dump();
  static void changeWindow(Window* w);
  static void createchar(u16* tgt, const char* descr, u8 fg=15, u8 bg=0);
  static void createchar16(u16* tgt, const char* descr, u8 fg=15, u8 bg=0);
  static void drawPattern(const char* pattern, int offset=0, u16* vram=WIDGETS_BACKGROUND);
};

class UpWindow : public Window {
protected:
  static volatile u32* LAYERS; //=REG_DISPCNT
  static u16* vram; //=WIDGETS_OVERLAY;
public:
  UpWindow(LayersConfig* lc = 0) : Window(lc) { }
  virtual Widget* add(screen_pixel_t x, screen_pixel_t y, Widget* w) {
    isdown=false;
    u16* vr=vram+(y/8)*32+(x/8);
    WidgCoords wc(w,x,y);
    w->place(vr,wc);
    obj.push_back(wc);
    nbWidgets++;
    return w;
  }
};

class DownWindow : public Window {
protected:
  static volatile u32* LAYERS; //=REG_DISPCNT
  static  u16* vram;
public:
  DownWindow(LayersConfig *lc = 0) : Window(lc) { }
  inline Widget* add(screen_pixel_t x, screen_pixel_t y, Widget* w) {
    isdown=true;
    u16* vr=vram+(y/8)*32+(x/8);
    WidgCoords wc(w,x,y);
    w->place(vr,wc);
    obj.push_back(wc);
    nbWidgets++;
    return w;
  }
};

/** a pseudo-window to capture 'guru mediation' customization
 */
class GuruWindow : public Window {
public:
  GuruWindow();
  /** will be invoked when a hardware exception is triggered
   */
  virtual void restore();
  virtual Widget* add(screen_pixel_t x, screen_pixel_t y, Widget* w) { return w; }
  virtual void release() {}
};

#define CONCLEAR "\x1b[2J"
#define CONGOTO  "\x1b[%d;%dH"
#define CON_GOTO(r,c) "\x1b["#r";"#c"H"

#endif
