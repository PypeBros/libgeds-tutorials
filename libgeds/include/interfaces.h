/* -*- c++ -*- */
#ifndef __GAME_INTERFACES__
#define __GAME_INTERFACES__
#include <sys/types.h>
#include "CoreTypes.h"
#include "Animator.h"
#include "Exceptions.h"
#include "nocopy.h"
class SpriteSet; // defined in SpriteSet.h.
class SpriteRam;

/** bits describing how the world behaves.
    better avoid using 0x8000 for signedness conflicts awaits you at the end **/
enum tile_properties {
  F_PLAYERTHRU=1,
  F_BLOCKING  =2,
  F_FLOOR     =4,
  F_LSLOPE    =8,  // is higher on the left
  F_RSLOPE  =0x10, // is higher on the right.
  F_MONSTERTHRU=0x20,
  F_WATER      =0x40,
  F_FALLTHRU   =0x80,
  F_CLIMB      =0x100,
  F_NORAISER     =0x4000,
};
/** \see tile_properties */
typedef unsigned tile_properties_t;

class GameObject;

/** an abstraction of the level map, as used by the controllers.
 */
class iWorld {
 public:
  static tile_properties_t properties[];
  virtual tile_properties_t getflags(world_tile_t tx, world_tile_t ty, GameObject *who=0)=0;
  virtual ~iWorld() { /*iprintf("world destroyed\n");*/ };
};
#define __I_WORLD__

#include <string>
#include <map>

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

class iGobController;
#include "GameObject.h"

class iControllerFactory {
  NOCOPY(iControllerFactory);
  static iControllerFactory* allUnregistered;
  iControllerFactory* nextUnregistered;
protected:
  std::string name;
public:
  iControllerFactory() : nextUnregistered(allUnregistered),
			 name()
  {
    allUnregistered = this;
  }
  const std::string getname() { return name;}
  static bool registerAll();
  virtual iGobController* create(char* args)=0;
  virtual ~iControllerFactory() {}
};

enum iThought { NONE=0, EVENT=1, FAIL=2, INIT=8 };

/** defines something to do with a Gob at every frame */
class iGobController : protected UsingScript, UsingTank {
  NOCOPY(iGobController);
  /** rather than creating complex and complete controllers, let's chain simple operations,
   *  at GobScript level.
   */
  iGobController* next;
  const iControllerFactory* madein;
public:
  void* operator new(std::size_t sz);
  void operator delete(void *ptr);
  iGobController(const iControllerFactory *icf) 
    : next(0), madein(icf)
  {
  }

  /** add a new controller to the chain */
  void enqueue(iGobController* ct) {
    if (next) next->enqueue(ct);
    else next=ct;
  }
  /** call the next controller in the chain, then combine their thoughts
   *   to indicate to the game engine what transitions should be performed.
   */
  inline iThought combine(iThought a, s16 gob[8], GameObject *g) {
    iThought b = next?next->think(gob,g):NONE;
    return (iThought)((int)a | (int)b);
  }
  /** do the job for this frame. Tells whether we can stay in this state */
  virtual iThought think(s16 gob[8], GameObject *g)=0;
  virtual ~iGobController() { if (next) delete next;}
};


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

  static const uint tile_index = 2;
  static const uint map_index = 8;
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
