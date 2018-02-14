/* -*- C++ -*- */
#ifndef __DEBUG_H
#define __DEBUG_H
extern bool debug_mode;
#define dprint if (debug_mode) iprintf
#endif

extern "C" {
  void die(const char* file, int line); /** you should define this in main to handle errors **/
}


#ifndef __I_REPORT
#define __I_REPORT
class iReport {
  static char repbuf[4096];
  static int reppos;
  static int warnings;
public:
  static void warn(const char* fmt, ...);
  static void action(const char* fmt, ...);
  /* reports an issue and returns false. */
  static bool report(const char* fmt, ...);
  static void step(const char* fmt, ...);
  static void diagnose();
  virtual ~iReport() {}
};
#endif


#ifndef __CHIEF_INSPECTOR
#define __CHIEF_INSPECTOR

#ifndef __GUI_ENGINE_H
#include "GuiEngine.h"
#endif
class Widget; class Window;
class InspectorWidget;
class GameObject;
class GobTransition; class GobArea; class iGobController; 
class GobCollision;

class ChiefInspector {
  static GameObject* suspect;
  static InspectorWidget *widget;
  static void report(iGobController *gc, u8 state, u32 msg);
public:
  static Widget* create(Window* game);
  static void suspects(GameObject *g);
  static void inspect();
  static void inspect(GameObject *g);
  static void inspect(const GobTransition *gt, const GobCollision *gc);
  static void inspect(const GobArea *, GameObject* active, GameObject* passive);
  static void step();
  static void report(GameObject *g, iGobController *gc, u8 state, u32 msg, const char* longstate) {
    if (g==suspect) report(gc,state,msg);
  }
  static bool nosuspect() {
    return (suspect==0);
  }
  static GameObject *now_suspect() {
    return suspect;
  }
  static const char** mainArea();
  static bool fullquiet;
};

#endif
