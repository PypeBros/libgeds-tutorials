/* -*- c++ -*- */
#ifndef __GAME_INTERFACES__
#error "you should include interfaces.h first"
#endif

#ifndef __GAME_OBJECT
#define __GAME_OBJECT
typedef int cflags;
#include <list>
#include <vector>

#include "debug.h"

template <typename Tank>
class GobExpression;
// see StateMachine.h
class GobTransition;
class GobState;
struct GobCollision;

/** "gob registers" assignments agreed across controllers. **/
enum gobdata {
  GOB_XSPEED=0,
  GOB_YSPEED,
  GOB_RESERVED1,
  GOB_DIRS, // A B slST> < ^ v R L X Y - - - - T c  
};

/** this is a game object (entity) as the rest of the game engine knows it.
    it has cdata that can be manipulated by controllers, a position, a size
    and a cast: everything you need to control and run collision tests on it.
*/
class GameObject : public iDebugable {
  NOCOPY(GameObject);
  friend class ScriptParser; // ::GobCommand(char *l);
#ifdef GLOBAL_DEBUGGER
  friend GLOBAL_DEBUGGER;
#endif

public:
  typedef std::list<GameObject*> GobList;
  //!< imported from the GobState.

  enum CAST { HERO, 
	      EVIL, 
	      DYNAMIC, 
	      DONTLIST, 
	      NBLISTS=DONTLIST
  }; 
private:
  static GobList gobs[NBLISTS];
  GobList::iterator self; 
protected:
  static GobTransition **doevents;
  static bool debugging;
  static int paused;
  static int raiser_flag;
  // public: // <-- or friend of GameScript.
  s16 cdata[16]; //!< 8 words of data that are controlled by the engine
                 //!< the next 8 words are free to use for #GobExpression
  int hotx, hoty; //!< hotspot (pixels) for slope movements.
protected:
  int x, y;             //!< coordinates (sub-pixel)
  int wbox, hbox;       //!< hit box (pixels)
  int gobno;            //!< index in the GameScript -1 for dynamic gobs.

  bool focus;  // the object is followed by the camera
  bool raise;  // the object should be displayed at a higher layer
  bool mayfreeze; // the object may stop working when going out of range

public:
  static const char* force; //! use this as message to dump() to force display.

  /** Casts "Hero and Evil" are used for collision detection, and indicate which
   * linked list the Gob belongs to. Gobs that do not *receive* collisions either
   * belong to "dynamic" or "dontlist" depending on whether they have been 'shot'
   * by another gob or were defined on the level map.
   */
  enum CAST cast;

  /** captures the transition list associated with an external event source
   * (i.e. a GobController)
   * /!\ assumes no re-entrance until gobRunController evaluates the result.
   **/
  static void useEvent(GobTransition **evt) {
    doevents = evt;
  }

  /** scan through HERO, EVIL and DYNAMIC lists, and delete all the 
   *  gobs that are still present.
   */
  static void clearDynGobs();

  /** forces the Gob to appear above both BG planes */
  void setraise(bool r=true) { raise=r; }
  /** forces the Gob to get the camera focus */
  void setfocus(bool f=true) { focus=f; }
  /** do we have camera focus ? */
  bool isfocus() {return focus;}
  static void setdebug(bool t=true) { debugging=t; }
  /** returns X pixel coordinate of the Gob */
  inline int px() const { return x>>8; }
  /** returns Y pixel coordinate of the Gob */
  inline int py() const { return y>>8; }

  static void pause(int val) {
    paused=val;
  }

  void getspeeds(s16 *into){
    into[GOB_XSPEED]=cdata[GOB_XSPEED];
    into[GOB_YSPEED]=cdata[GOB_YSPEED];
  }

  GameObject(CAST c, int gno=-1, const char *pname="gob.");
  virtual ~GameObject();
  void setfreezable(bool canfreeze) {
    mayfreeze = canfreeze;
  }
  virtual void setxy(int _x, int _y)=0;
  void setbbox(int w, int h);

  inline const void getcoords(int &_x, int &_y) {
    _x=x;
    _y=y;
  }

  /** get pixel coordinates of collision boundary box into @_x and @_y of
   *  - top-left corner when center=0 (default),
   *  - center when center=1,
   *  - bottom-right corner when center=2.
   */
  inline const void getpxcoords(int &_x, int &_y, int center=0) {
    _x=(x>>8)+(wbox*center)/2;
    _y=(y>>8)+(hbox*center)/2;
  }

  static GameObject* CreateSimpleGob(CAST _cast, GobState *init, int gno = -1);
};
#endif
