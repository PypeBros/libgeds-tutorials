/* -*- c++ -*- */
#ifndef __GAME_INTERFACES__
#define __GAME_INTERFACES__
#include <sys/types.h>
#include "Animator.h"
#include "Exceptions.h"
#include "nocopy.h"
class SpriteSet; // defined in SpriteSet.h.
class SpriteRam;

class GameObject;

#include <string>
// #include <map>

/** an abstract resource reader, returning contents line by line 
 */
class InputReader {
public:
  virtual int readline(char *l, int sz)=0;
  virtual ~InputReader() {}
};

/** An allocator helper, that keeps all the provided objects as a "generation"
 *  in one tank until the whole tank is trashed
 */
class GobTank;

/** the base class for things that need to be allocated in the tank, 
 *  so that they have access to its 'singleton' pointer. And only them
 */
class UsingTank {
protected:
  static GobTank* tank;
public:
  static void settank(GobTank *tk);
  virtual ~UsingTank() {}
};

/** a tank-like storage provider that will use Malloc.
 */
class Mallocator;
/** the base class when we want to use a class that could be in the tank 
 *  with the regular memory allocator
 */
class UsingMalloc {
protected:
  static Mallocator* tank;
};

/** the generic interface for a scripted level */
class iScript;

/** the base class for the things that need access to the singleton scripted level */
class UsingScript {
protected:
  static iScript* script;
public:
  static void setscript(iScript *scr) {script=scr;}
  virtual ~UsingScript() {}
};
 
class iDebugable : public Animator {
 public:
  iDebugable(int n, const char* pnme="----") : Animator(n, pnme) {}
  virtual void dump(const char* why=0)=0;
  virtual ~iDebugable() {}
};

#include "GameObject.h"

enum iThought { NONE=0, INIT=8 };

/** how to know when a level change starts or ends
 */
class iScriptListener {
public:
  virtual void changeLevel()=0;
  virtual void levelReady()=0;
  virtual ~iScriptListener() {}
};

typedef enum {
  FOREGROUND_LAYER=0, // playground, occludes player
  BACKGROUND_LAYER=1, // playground, behind player
  FARGROUND_LAYER=2,  // parallax.
  FURGROUND_LAYER=3,  // even more parallax if 3D is disabled
  NB_BGLAYERS=4,
  EMBEDDED=8
} bglayer_t;

typedef enum {
  GOB0=0
} gobno_t;

/** abstract version of a script-based level */
class iScript {
 protected:
public:

  static const uint tile_index;
  static const uint map_index;
  //! continuously parse until a message is printed.
  virtual bool parsechunk()  throw(iScriptException) = 0;
  
  virtual bool setGobState(gobno_t target, unsigned stateno) = 0;

  //! should we play another level ? which one ?
  //  virtual char *getNextLevel(int id) = 0;

  virtual SpriteSet* getSpriteSet(bglayer_t id)=0;
  //! stop all Animators and whatever could produce automatic screen update
  virtual void stop()=0;
  
  virtual ~iScript() {};

  //! builds a real instance of GameScript, returns the iScript interface.
  /*! may recycle the current GameScript instance when 'reload' mode is used,
   *  as defined by the last loading request.
   */
  static iScript* create(InputReader *ir);
  static iScript* running(); //!< returns the running script.

  virtual GameObject *getgob(gobno_t i)=0;
  virtual GameObject *getfocused()=0;
  //! same as a "gob* : stateX (0,0) command.
  virtual void regListener(iScriptListener *l)=0;
};

#define __I_SCRIPT__

#endif
