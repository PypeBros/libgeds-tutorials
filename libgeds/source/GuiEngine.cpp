#include <nds.h>
#include "GuiEngine.h"
#include <stdio.h>

bool debug_mode=false;

void break_here() {
}

u16* DownWindow::vram=SUB_WIDGETS_TEXT;
u16* UpWindow::vram=WIDGETS_TEXT;
volatile u32* DownWindow::LAYERS=&REG_DISPCNT_SUB;
volatile u32* UpWindow::LAYERS=&REG_DISPCNT;

Engine* Engine::singleton=0;
Engine *theEngine=0;

class Nowidget :public Widget {
public:
  Nowidget() : Widget(0) {}
  virtual void place(u16* vr, struct WidgCoords &wc) {}
  virtual void render(void) {}
  virtual bool handle(uint x, uint y, Event evt) {return false;}
} nowidget;

unsigned Window::setmark(uint mode/*=END_OF_LINE*/) {
  WidgCoords w(&nowidget,0,0);
  w.setup(0,0,mode);
  obj.push_back(w);
  return nbWidgets++;
}

int Window::help(int no) { 
  iprintf("     ** no help here yet **\n\n\n\n"
	  "http://sylvainhb.blogspot.com\n");
  return 0; 
}


void Window::render(void) {
  if (super) super->render();
  for (int i=(int)wstart;i!=(int)nbWidgets && i<(int)obj.size();i++) {
    if (obj[i].last()) break;
    if (wstart>nbWidgets && obj[i].chain()) { 
      i=0;
    }
    obj[i].render();
  }
}

/** identify the WidgCoords that corresponds to a specific position
 */
WidgCoords* Window::test(touchPosition touch) {
  for (uint i=wstart;i!=nbWidgets && i<obj.size();i++) {
    if (obj[i].last()) break;
    if (wstart>nbWidgets && obj[i].chain()) { 
      i=0;
    }
    if (obj[i].test(touch)) return &(obj[i]);
  }
  return 0;
}

/** retrieves the coordinates of a given widget 
 */
WidgCoords* Window::widget(Widget* w) {
  for (uint i=0;i<obj.size();i++)
    if (obj[i].samewidget(w))
      return &(obj[i]);
  return 0;
}


void Engine::enable3D() {
  static bool enabled3D = false;
  REG_DISPCNT|=ENABLE_3D|DISPLAY_BG0_ACTIVE;

  if (enabled3D) return;
  /** delegates components powering (matrices & 3D core), setup of GL matrices
   *  etc. to glInit() in the ndslib.
   */
  glInit();
  glClearColor(0,0,0,0); // BG must be opaque for AA to work
  glClearPolyID(63); // BG must have a unique polygon ID for AA to work
  glClearDepth(0x7FFF);
  glViewport(0,0,255,191);
  
  //this should work the same as the normal gl call
  // no effect. glViewport(0,0,255,191);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  
  glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
  
  /** make the screen covered by [-2,1.5]-[2,-1.5] */
  gluPerspective(112, 256.0 / 192.0, 0.1, 40);
  gluLookAt(      0.0, 0.0, 1.0,          //camera possition 
		  0.0, 0.0, 0.0,          //look at
		  0.0, 1.0, 0.0);         //up
  glMatrixMode(GL_MODELVIEW);

  enabled3D = true;
  
  /** lights definitions are needed too. */
}



u16* Engine::editpal(uint bg, bool edit) {
  if (singleton->config&EXTENDED_SPRPALS) {
    if (edit) vramSetBankF(VRAM_F_LCD);
    else vramSetBankF(VRAM_F_SPRITE_EXT_PALETTE);
  }
  if (singleton->config&EXTENDED_PALETTES) {
    bg&=3; /** bg is supposed to be between 0 and 3 **/
    if (edit) 
      vramSetBankE(VRAM_E_LCD);
    else 
      vramSetBankE(VRAM_E_BG_EXT_PALETTE);
    return VRAM_E + 256*16*bg;
  } else {
    return BG_PALETTE;
  }
}

/** The GuiEngine maintains a stack of 'active' windows, identified 
 * from the current (top-of-stack) window 'interface' and all its
 * super-windows.

 * restore() is invoked as result of a setWindow() on windows that
 *   are newly activated (that is, those that join the stack)

 * release() is invoked on windows that leave the stack.
 *   note that release can only be invoked _out_ of animations. If
 *   setWindow() is invoked from an Animation, the whole window
 *   switching process is deferred and will automatically happen
 *   at the end of Enine::animate() function.
 *

 *  X -> subX : X is not released.
 *  subX -> X : just screen refresh. subX released, of course.
 *  subX -> Y : both subX and X must be released !
 *  subX -> osubX : only subX should be released, X is kept.
 * /!\ subX -> subY : not currently supported.
 */

void Engine::setWindow(Window *itf)  { 
  current=0;

  // not processed if switching to a sub-window.
  Window *w;
  
  for (w = interface; w && w!=itf && !itf->issubwin(w); w=w->holder()) {    
    w->release();
  }
  
  if (itf) itf->clearscreen();  // we did some change.
  if (w!=0) w->render();            // some window is still active.


  if ((interface=itf)) {
    swap_colors = interface->swap_colors && (config&SWAP_COLORS_ON_L);
    if (interface != w) interface->restore();
    interface->render();

  }

  if (itf->isdown) lcdMainOnTop();
  else lcdMainOnBottom();
}

/** force a widget of the current window to get the focus (i.e. be current) */
bool Engine::focus(Widget *w) {
  if (!interface) return false;
  current=interface->widget(w);
  return current!=0;
}


/*static*/ 
void Engine::changeWindow(Window* w) {
  if (singleton) singleton->setWindow(w);
}

/*static*/
void Engine::drawPattern(const char* pattern, int offset, u16* vr) {
  vr+=offset;
  u16 *top=vr;
  for (;*pattern;pattern++) {
    if (*pattern==' ') {
      top+=4;
      vr=top;
    } else {
      int d=*pattern-'0';
      int h=*pattern-'a';
      if (d<0 || h>6) {
	iprintf("invalid pattern char %c (%i,%i).\n",*pattern,d,h);
	break;
      }
      if (d>9) d=10+h;
      vr[0]=(d&8)?Widget::BOFF:Widget::BCHECK;
      vr[1]=(d&4)?Widget::BOFF:Widget::BCHECK;
      vr[2]=(d&2)?Widget::BOFF:Widget::BCHECK;
      vr[3]=(d&1)?Widget::BOFF:Widget::BCHECK;
      vr+=32;
    }
  }
}

void Engine::handle(void) {
  Event evt=GUI_NOPE;
  // read the button states
  scanKeys();
  

  // read the touchscreen coordinates
  touchPosition touch;
  uint pressed = keysDown();	// buttons pressed this loop
#ifdef HARDTODEBUG
  volatile uint press = pressed;
#endif
  held = keysHeld();	// buttons currently held
  /* when the touchpad is released, touchReadXY returns (0,0).
   * we prefer working on the last coordinates in that case.
   */
  if (lefthand) {
    /* KEY_R = BIT(8) ; KEY_L = BIT(9) */
    uint lrpress = ((pressed&KEY_L)>>1)|((pressed&KEY_R)<<1);
    uint lrheld = ((held&KEY_L)>>1)|((held&KEY_R)<<1);
    pressed = (pressed&~(KEY_L|KEY_R))|lrpress;
    held = (held&~(KEY_L|KEY_R))|lrheld;
  }
  
  if (swap_colors && ((pressed|held)&KEY_L)!=(lastbuttons&KEY_L)) {
    u16 col=BG_PALETTE_SUB[0xfd];
    BG_PALETTE_SUB[0xfd]=BG_PALETTE_SUB[0xfc];
    BG_PALETTE_SUB[0xfc]=col;
  }

  if (pressed&KEY_TOUCH) {
    touchRead(&touch);
  } else if (held&KEY_TOUCH) {
    touchRead(&touch);
  } else {
    touchPosition nil;
    touch=lastpos;
    touchRead(&nil);
  }

  uint16 mask;
  bool inside = current && current->test(touch);
#ifdef HARDTODEBUG
  (void) press;
#endif
  if ((pressed&KEY_TOUCH)) {
    /* a new touch starts. If we're no longer in the previous
     * widget, let's just drop it to get something fresh ...
     */
    evt=(held & KEY_L)?GUI_ALT_TOUCHED:GUI_TOUCHED;
    if (!inside) current=0;
    lastpos=touch;
  } else {
    if ((held&KEY_TOUCH)) {
      /* we're drawing with the pencil. ignore movements that get out
       * until they're back in.
       */ 
      evt=(held&KEY_L)?GUI_ALT_TOUCHED:GUI_MOVED;
      lastpos=touch;
      if (!inside) {
	lastbuttons = held|pressed; return;
      }
    } else if (lastbuttons&KEY_TOUCH) {
      /* touching inside and releasing outside is not clicking */
      evt = inside?((held & KEY_L)?GUI_ALT_CLICKED:GUI_CLICKED):GUI_RELEASED;
    }
  }

  bool done=false;

  for (Window* win=interface;win && !done;win=win->holder()) {
    /* now, if we have no longer a current object, let's check for one 
     * XXX 28.09.2009: we're better not looking for a new current widget
     * unless we have a STYLUS event. That would screw up selection of
     * widgets when new windows are selected. 
     */
    if (!current && win->active && ((pressed&KEY_TOUCH)||(held&KEY_TOUCH))) {
      current = win->test(touch);
      inside=true;
      mask=GUI_NOPE;
    }
    /* at this point, if we still have a current object, it means it's
     * eligible for event delivery ... we no longer care wether we're 
     * inside or not
     */
    if (current) {
      mask=current->getMask();
      if (evt&mask) done=current->handle(touch,evt);
    } else {
      mask=GUI_NOPE;
    }
    
    /** unhandled button presses are deferred to the window.
     **/
    
    if (pressed&(KEY_DPAD)) {
      if (!(mask&GUI_DIRECTION) || !current || 
	  !(done=current->handle(pressed,GUI_DIRECTION)))
	done|=win->handle(pressed,GUI_DIRECTION);
    }
    if (pressed&(KEY_ABXY)) {
      if (!(mask&GUI_BUTTON) || !current || !(done=current->handle(pressed,GUI_BUTTON)))
	done|=win->handle(pressed, GUI_BUTTON);
    }
    if (pressed&(KEY_L|KEY_R|KEY_START|KEY_SELECT|KEY_LID)) {
      if (!(mask&GUI_SPECIAL) || !current || !(done=current->handle(pressed,GUI_SPECIAL)))
	done|=win->handle(pressed, GUI_SPECIAL);
    }
    uint allkeys = KEY_DPAD|KEY_ABXY|
      (KEY_L|KEY_R|KEY_START|KEY_SELECT);
    uint prheld = pressed|held;
    if ((allkeys&prheld)!=(lastbuttons&allkeys) && win->transient)
      done|=win->handle(prheld, GUI_ANYKEY);
  }
  lastbuttons = held|pressed;

}

unsigned int currentTime = 0;
// a very simple way to "hide" the debug console: we give it priority lower tha
// n background.
// that way, we can recall it whenever we want.
void Engine::DebugConsole(bool showit) {
  if (showit) REG_BG0CNT = BG_MAP_BASE(GECONSOLE)|BG_TILE_BASE(0)|BG_PRIORITY(0);
  else        REG_BG0CNT = BG_MAP_BASE(GECONSOLE)|BG_TILE_BASE(0)|BG_PRIORITY(3);
}
void VBlankHandler()
{
	currentTime++;
}

void Engine::showSprites(bool show)
{
  if (show) {
    REG_DISPCNT |= 
      DISPLAY_SPR_ACTIVE|
      DISPLAY_SPR_1D |        //this is used when in tile mode
      DISPLAY_SPR_1D_SIZE_64;
    REG_DISPCNT_SUB |=
      DISPLAY_SPR_ACTIVE|
      DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64;
    dprint("sprites shown");
  } else {
    REG_DISPCNT &= DISPLAY_SPR_ACTIVE;
    REG_DISPCNT_SUB &= DISPLAY_SPR_ACTIVE;
    dprint("sprites hidden");
  }
  
}

void vcountirq() {
  WIDGETS_CONSOLE[0]++;
  // HOOK:  iVcountCallback::vcountAll();
}

void Engine::prepare(void)
{
  powerOn(POWER_ALL_2D);
 
  //irqs are nice
#ifdef __DEVKITPRO_R21
  /** yet, the irq system is now initialised prior to the execution of main(),
   **  including some interrupts used in 7-of-9 synchronisation. Calling irqInit
   **  out of the lib will throw chaos and madness in the place.
   **/
  irqInit(); 
#endif
  irqSet(IRQ_VBLANK, VBlankHandler);
  // HOOK: irqSet(IRQ_VCOUNT, vcountirq);
  irqEnable(IRQ_VBLANK);
  
  vcno=0;

  //set the video mode
  videoSetMode(  MODE_0_2D | 
		 DISPLAY_SPR_ACTIVE |    //turn on sprites
		 DISPLAY_BG3_ACTIVE |   
		 DISPLAY_SPR_1D |        //this is used when in tile mode
		 DISPLAY_SPR_1D_SIZE_64
		 );

  videoSetModeSub(((config&CONSOLE_DOWN)?MODE_0_2D:MODE_1_2D) | 
		    DISPLAY_SPR_ACTIVE|
		  /** even if sprites are made of 8x8 tiles, we have only 10 bits of 
		   *  'tile selection' in the OAM attributes. That means we couldn't
		   *  use more than 32KB of memory if we stick to GBA's "32 bytes per
		   *  tile" mechanism (that makes sense for 16-colour sprites, if you
		   *  ask). Here, we'd like to keep our mind sane and use 1:1 mapping
		   *  between "tile id" and "tile VRAM". 
		   *  DISPLAY_SPR_1D_SIZE_32: fine for 16-colours if you need tile-granularity
		   *  DISPLAY_SPR_1D_SIZE_64: tile-granularity for 256-colours sprites
		   *  DISPLAY_SPR_1D_SIZE_128: vram = index*2. 'odd' tiles cannot be used alone,
		   *    but that lets us use the whole 128KB of sprites tiles memory on engine B
		   *  DISPLAY_SPR_1D_SIZE_256: vram = index*4. TeH only way to use full 256 VRAM
		   *    for sprites on engine A.
		   */
		  DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64
		  );   // the background.
  switch(config&MEMORY_SETTINGS) {
  case NORMAL:
    //enable vram and map it to the right places
    vramSetPrimaryBanks(VRAM_A_MAIN_SPRITE,        // flat allocation: everything has 128K
			VRAM_B_MAIN_BG_0x06000000,        
			VRAM_C_SUB_BG_0x06200000,  //map C to background memory
			VRAM_D_SUB_SPRITE
			); 
    break;
  case BG256_SPR64_MEMORY:
    vramSetPrimaryBanks(VRAM_A_MAIN_BG_0x06000000,
			VRAM_B_MAIN_BG_0x06020000, // 256K on main BG
			VRAM_C_SUB_BG_0x06200000,  //map C to background memory
			VRAM_D_SUB_SPRITE
			); 
    vramSetBankE(VRAM_E_MAIN_SPRITE);              // but only 64K sprites.
    break;
  case MORE_BG_MEMORY:
    vramSetPrimaryBanks(VRAM_A_MAIN_SPRITE,           // 256K for main BG.
		     VRAM_B_MAIN_BG_0x06000000,     // 128K for others
		     VRAM_C_SUB_BG_0x06200000,
		     VRAM_D_MAIN_BG_0x06020000
		     );
    vramSetBankI(VRAM_I_SUB_SPRITE); // watch out: only 16K for SUB sprites
    break;
  case MORE_SPR_MEMORY:
    vramSetPrimaryBanks(VRAM_A_MAIN_SPRITE, // 256K for main SPR
		     VRAM_B_MAIN_SPRITE_0x06400000, // 128K for others
		     VRAM_C_SUB_BG_0x06200000,
		     VRAM_D_MAIN_BG_0x06000000
		     );
    vramSetBankI(VRAM_I_SUB_SPRITE); // watch out: only 16K for SUB sprites
    break;
  }
  if (config&EXTENDED_PALETTES) {
    vramSetBankE(VRAM_E_BG_EXT_PALETTE);
    REG_DISPCNT|=DISPLAY_BG_EXT_PALETTE; // turn it on.
  }
  if (config&EXTENDED_SPRPALS) {
    vramSetBankF(VRAM_F_SPRITE_EXT_PALETTE);
    REG_DISPCNT|=DISPLAY_SPR_EXT_PALETTE;
  }
  for (int i=0;i<65536;i++) {
    BG_GFX_SUB[i]=0;
    BG_GFX[i]=0;
  }

#ifdef __NO_CONFIG__
  //enable vram and map it to the right places
  vramSetPrimaryBanks(VRAM_A_MAIN_SPRITE,        
		      VRAM_B_MAIN_BG_0x06000000, // 128K for the main background ...
		      VRAM_C_SUB_BG_0x06200000,  //map C to background memory
		      VRAM_D_SUB_SPRITE
		      ); 
#endif

  /* consoleInit automatically invokes bgInit() to setup console. */
  consoleInit(consoleGetDefault(),
	      (config&CONSOLE_DOWN)?3:0, // layer
	      BgType_Text4bpp, //type
	      BgSize_T_256x256,//size
	      GECONSOLE,       // mapbase
	      0,               //tile
	      (config&CONSOLE_DOWN)!=0,
	      true);
  consoleInit(consoleGetDefault(),
	      (config&CONSOLE_DOWN)?0:3, // layer
	      BgType_Text4bpp,
	      BgSize_T_256x256,
	      GECONSOLE,
	      0,
	      (config&CONSOLE_DOWN)==0,
	      true);

   // black backdrop
  BG_PALETTE[0]=RGB15(0,0,0);
 
 

  BG_PALETTE[255] = RGB15(31,31,31);//by default font rendered with color 255
  BG_PALETTE_SUB[0xff]=RGB15(31,31,31);
  SPRITE_PALETTE[0xff]=RGB15(31,31,31);
  SPRITE_PALETTE_SUB[0xff]=RGB15(31,31,31);

  // HOOK: UsingSprites::DisableAll();
  
  memset(WIDGETS_BACKGROUND, 0, 32*32*4*sizeof(u16));
  memset(SUB_WIDGETS_BACKGROUND, 0, 32*32*4*sizeof(u16));

  // filling characters 0..15 as being squares of colors 0..15 resp.
  // 8x8 pixels, 2 pixels per byte -> 16 u16 per character in 16-color mode.
  // we can show more colors by picking another palette...
  for (int i=0;i<15;i++) {
    memset(((u16*)CHAR_BASE_BLOCK(0))+i*16,i|(i<<4),32);
    memset(((u16*)CHAR_BASE_BLOCK_SUB(0))+i*16,i|(i<<4),32);
  }


  // HOOK: UsingSprites::Init();
  
  if (!singleton) {
    singleton=this;
    theEngine=this;
  }
  magic=0xcafebabe;
  iprintf("PPP Engine v2.0- ready\n");
}
  


void Engine::sanitize() {
  // HOOK: UsingSprites::sanitize();
}

#ifdef DYNAMIC_ENGINE
Engine::~Engine() {
  if (this==singleton) {
    singleton=0;
    theEngine=0;
  }
}
#endif

Engine::Engine(enum GuiConfig cfg, Window *itf,u16 bgcolor) : 
  interface(itf), nextWindow(0), current(0), lastpos((touchPosition){0xffff,0xffff,0,0,0,0}),
  lastbuttons(0), 
  magic(0xcafebabeUL), /*static held(0),*/ 
  lefthand(false), config(cfg), 
  swap_colors(false), vcno(0)
{
  if (singleton) {dprint(">_< we already have an engine");}
  else {
    singleton=this;
    theEngine=this;
  }
  if (interface) interface->render();
  /* weirdo. it looks like this constructor is called, but than
   * the state of ge is then lost ???
   */

  if (bgcolor) {
    BG_PALETTE_SUB[0xfd]=bgcolor;
    BG_PALETTE_SUB[0xfc]=RGB15((bgcolor&31)*3/4,
			       ((bgcolor>>5)&31)*3/4,
			       ((bgcolor>>10)&31)*3/4);
  }
}

//! run animation list
/* this one is static too */
void Engine::animate(void) {
  // HOOK: Animator::Animate();
  // invoke registered post-VBL callbacks.
  // HOOK: iVblCallback::runAllPost();
  glPopMatrix(1);
  glFlush(0);

  sync();
  // HOOK: iVblCallback::runAllVbl();
  // HOOK: Animator::ProcessPending();
  if (nextWindow) setWindow(nextWindow);
  
}

void Engine::sync(void) {
  // make sure we flushed the cache for OAM copy 
  //  DC_FlushRange(Engine::sprites,256*sizeof(SpriteEntry));
  ///* if (use3D) */glFlush();

  swiWaitForVBlank();
  // HOOK: UsingSprites::SyncToOam();
}

void Window::dump() {
  dprint("- %i widgets\n",nbWidgets);
}

void Engine::dump() {
  debug_mode=true;
  sanitize();
  
  //  dprint("registered: %i+%i OAMs\n",singleton->rescount[RES_OAM],singleton->rescount[RES_OAM_SUB]);
  //  dprint("animation list starts at %p\n",singleton->todo);
  dprint("interface is %p\n",singleton->interface);
  dprint("current widget is %p\n",singleton->current);
  dprint("last click at (%i,%i)\n", singleton->lastpos.px, singleton->lastpos.py);
  singleton->interface->dump();
}

// unpacks a 16 digits that encodes a 8x8 bit matrix in hex.
/* e.g. "183c667E6666" is an "A", "3c66666e76663c" is a 0, etc ...
 *      "0102040810204080" is a thin /, "ffffffffffffffff" is of course
 *      full 'fg'. fg defaults to 15 and bg defaults to 0 (transparent)
 *      note that BG256 is assumed.
 */
void Engine::createchar(u16* tgt, const char* descr, u8 fg, u8 bg) {
  u16 line[2];

  for (int i=0;i<16;i++) {
    char c=*descr++;
    //    line[0]=0; line[1]=0;
    if (c>='0' && c<='9') c=c-'0';
    else c=10+c-(c>'F'?'a':'A');
    if (c>16) c=0;
    
    line[0]=(c&4)?(fg<<8):(bg<<8);
    line[0]|=(c&8)?fg:bg;
    line[1]=(c&1)?(fg<<8):(bg<<8);
    line[1]|=(c&2)?fg:bg;
    *tgt++=line[0];
    *tgt++=line[1];
  }
  
}

// unpacks a 16 digits that encodes a 8x8 bit matrix in hex.
/* e.g. "183c667E6666" is an "A", "3c66666e76663c" is a 0, etc ...
 *      "0102040810204080" is a thin /, "ffffffffffffffff" is of course
 *      full 'fg'. fg defaults to 15 and bg defaults to 0 (transparent)
 *      note that BG16 is assumed.
 */
void Engine::createchar16(u16* tgt, const char* descr, u8 fg, u8 bg) {
  u16 line; // one u16 will hold a whole digit. fine.

  fg&=0xf; bg &=0xf;
  
  for (int i=0;i<16;i++) {
    char c=*descr++;
    //    line[0]=0; line[1]=0;
    if (c>='0' && c<='9') c=c-'0';
    else c=10+c-(c>'F'?'a':'A');
    if (c>16) c=0;
    
    line=0;
    line=(c&8)?fg:bg;
    line|=(c&4)?(fg<<4):(bg<<4);
    line|=(c&2)?(fg<<8):(bg<<8);
    line|=(c&1)?(fg<<12):(bg<<12);
    *tgt++=line;
  }
  
}

vu16* saveit[]={
    &REG_BG0CNT, &REG_BG1CNT, 
    &REG_BG2CNT, &REG_BG3CNT,
    &REG_BLDALPHA, &REG_BLDCNT
};

uint Engine::held=0;

Widget::~Widget() {
}
