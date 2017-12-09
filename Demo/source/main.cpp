/*-----------------------------------------------------------------------------
  (c) Sylvain "Pype" Martin 2006-2010
  A demonstration game for GEDS. 
  This file is released to the public domain to ensure the L-GPL libgeds can
  be used correctly.
  -----------------------------------------------------------------------------*/
#include "nds.h"
#include <nds/arm9/console.h> //basic print funcionality
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fat.h>
#include <sys/dir.h>
#include <sys/types.h> 

/******************************************************************************
 ** Global project settings
 **----------------------------------------------------------------------------
 ** How is the game engine working : see the libgeds sub-project
 ******************************************************************************/


#include "GuiEngine.h"

/** NTXM is the sound library for playing XM files. it requires one instance of
 ** an NTXM9 class for control. See ntxm9.h for details.
 **/
#include <ntxm/ntxm9.h>
NTXM9 *ntxm9 = 0;
#define HAS_NTXM


/** Engine class captures the low-level interaction with the DS hardware,
 **  esp. it proceeds with video initialisation, is responsible of sprites 
 **  registers reporgramming on VBlank, etc.
 **/


/* each bit is one screen bit0 = GEBGROUND on main,
 * bit1 = GEWIDGET  on main, bit2 = GEYOURMAP, bit4 = GEBGROUND on SUB
 * Any screen with its bit set won't be affected by Engine::clearscreen
 */
Engine ge((GuiConfig)(CONSOLE_DOWN|MORE_BG_MEMORY));

class MetaWindow;

void iReport::diagnose() {
  iprintf("@_@\n@ %s %s",repbuf,repbuf+strlen(repbuf)+1);

  // show all debug.
  REG_BG1CNT=BG_MAP_BASE(GECONSOLE)|BG_TILE_BASE(0)|BG_PRIORITY(0); // keep the console on another layer.
  REG_BLDALPHA=0x1008;
  REG_BLDCNT = BLEND_SRC_BG1|BLEND_DST_BG2|BLEND_DST_BG0|BLEND_DST_BG3|BLEND_DST_SPRITE|BLEND_DST_BACKDROP|BLEND_ALPHA;

  die("diagnose", 0);
}

class MetaWindow : public DownWindow {
  NOCOPY(MetaWindow);
  /* declare your 'xxxWindow' objects here. */
  Window *active;
public:

  /** in the 'PPP Engine' framework, core activities are handled in Windows that can
   **  be stacked onto each other. The Meta Window is the top-level object that usually
   **  performs application-level initialisation (here, NTXM) and decide which part of 
   **  the application should run when. 
   **
   ** in our game, we have two main states : playing (the CmdWindow that executed .cmd
   **  files) or loading (the Loading Window). 
   **/
  MetaWindow(): active(0)
  {
    iprintf("creating windows -- ");
    ntxm9 = new NTXM9();
    active = this;
  }
  
  virtual bool event(Widget *w, uint id) {
    bool handled;
    /* handle buttons presses here. */
    ge.release();
    return handled;
  }
  

  void setactive() {
 
    /** there's only one music file in this project, and that's STRTRK.XM 
     **  the different sub-tunes and jingles are implemented by using loops
     **  using the Bxx module command that forces the player to jump to a
     **  specific location in the pattern-order-table. The game logic contains
     **  some track(xx) statements that forces the player to move out of the
     **  current loop on specific events.
     **/
    FileDataReader fd("efs:/sndtrk.xm");
    ntxm9->stop();
    u16 err = ntxm9->load(&fd);
    ge.setWindow(active);
    if (err!=0) iprintf("ntxm says %x\n",err);
  }


  bool handle(uint& keys, Event evt) {
    /** handle keypresses at window-level here.
     **  note that GameScript and DPadController already take in charge the
     **  processing of input for characters control. This is where you'd e.g.
     **  exit the game if L+R+START is held pressed
     **/
    if (keys & KEY_A) {
      ntxm9->play();
    }
    if (keys & KEY_Y) {
      ntxm9->stop();
    }
    if (keys & KEY_X) {
      int pat,row,chn;
      int code=ntxm9->getoops(&pat,&row,&chn);
      //      if (code)
      iprintf("%s(%x) @p%i:r%i:c%i\n",code?"oops":"okay",code,pat,row,chn);
      return true;
    }

    if (keys&KEY_B) {
      defaultExceptionHandler();
      iprintf("beware the guru\n");
      Engine::dump();
      
    }
    return true;
  }      

  virtual void restore() {
    iprintf("(A) play (Y) stop (X) info (B) dump\n");
  }
  
  virtual void release () {
    iprintf("releasing Meta window\n");

  } 
  
};

#include "efs_lib.h"

#include <cxxabi.h>

extern "C" std::type_info* __cxa_current_exception_type();

extern "C" void terminator() {
  std::type_info *t = __cxa_current_exception_type();
  iprintf("** exception %s reached top-level **\n", (t)?t->name():"unknown");
  die(0,0);
}

extern "C" void guruMeditationDump(); // should sit in libnds : gurumeditation.o

static void myGuruHandler() {
//---------------------------------------------------------------------------------
	guruMeditationDump();
	REG_DISPCNT &= ~DISPLAY_WIN0_ON;
	REG_DISPCNT &= ~DISPLAY_WIN0_ON;
	die("<unknown>",0);
}

/************************************************************************************
 ** So, this is where everything starts. Since C++ offers no guarantee on the
 ** order of initialisations, we tried to keep the constructors of global 
 ** objects slims and free of any side-effects. As a result, we need to call
 ** a number of prepare() functions before we actually proceed with proper code
 **
 ** Windows, however, build the state they need for their work in their constructor,
 ** so starting at MetaWindow mw; line, many things are put in place, that rely on
 ** ge.prepare() to be called before. 
 **/
int main(int argc, char* argv[])
{
  // note: the console is currently restricted to ASCII charset.
  std::set_terminate(terminator);
  ge.prepare();                      // make video hardware ready for operation
  ge.dump(); // enables debug.
  iprintf("PPP Team's runbox v 0.3 lite\n (c) sylvain 'pype' martin \n");
  MetaWindow mw;

  iprintf("beware the guru\n");
  defaultExceptionHandler();
  setExceptionHandler(myGuruHandler) ;

  /** the Embedded File System by NODA allows us to use libfat to access additional
   **  content present in the file without consuming additional RAM memory. smart.
   **/
  if (!(EFS_Init(EFS_AND_FAT|EFS_FAT_PRIO, (argc>0)?argv[0]:0)))
    iprintf("Cannot find data files (%i, %s)!\n",argc,argv[0]!=0?argv[0]:"#");
  else 
    iprintf(CONCLEAR "system initialisation completed");
    
  /** this is where the main meal starts. Esp, when MetaWindow::setactive() calls 
   **  GuiEngine::setWindow(active==CmdWindow), execution is resumed at CmdWindow::restore
   **  that makes the very first level (the menu) to be parsed and the game starts.
   **/
  mw.setactive();        

  while(1) {
    ge.handle();    // reads & decode inputs

    ge.animate();   // handle animation, including a wait for VBlank
  }
 return 0; // you're not supposed to reach this point.
}


/** these are heavy guys from the lib i want to strip out. **/
extern "C" char* _dtoa_r(_reent*, double, int, int, int*, int*, char**) {
  die(__FILE__, __LINE__);
  return (char*)"X.X";
} /** saves 397 - 388KB **/


/** slim fast :) **/
extern "C" char* __cxa_demangle(const char* mangled_name,
		       char* output_buffer, size_t* length,
		       int* status) {
  if (status) *status = -2;
  return 0;
} /* saves 403 - 388 KB */

