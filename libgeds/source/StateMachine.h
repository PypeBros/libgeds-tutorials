#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H
#include <vector>
#include <list>
#include <sys/types.h>
#include <nds.h>
#include "SpriteSet.h"
#include "GuiEngine.h"
#include "interfaces.h"
#include "SpriteAnim.h" // for iAnimUser
#include "GameObject.h"
#include "GameConstants.h"
#define MAX_LIMBS 12

#define STEPX 14  // within CDATA
#define STEPY 15

/** internal encoding of the operations for GobExpressions */
enum ActionOpcodes {
  OP_DONE=0, OP_NEG, OP_NOT, OP_INC, OP_DEC, OP_SCORE,
  OP_TRUE, OP_FALSE, OP_DEBUG, OP_FLICKER,
  OP_GETVAR=0x10,   //!< lower 4 bits encode which var we refer to.
  OP_SETVAR=0x20,   //!< see GETVAR
  OP_CONSTANT=0x30, //!< lower 4 bits encode which constant we want
  OP_ADD=0x40, OP_SUB, OP_MIN, OP_MAX,
        OP_OR, OP_AND, OP_XOR, OP_NAND,
       OP_MUL, OP_DIV, OP_MOD, // 0x3b-3f remain available
  OP_EQ=0x50, OP_NEQ, OP_LT,   OP_GT,
      OP_LTE, OP_GTE, OP_TEST, OP_NTST,

  OP_DUP=0x60, OP_SHL=0x70, OP_SHR=0x80,
};

enum AnimCommandType {
  ANIM_DONE,     // 00 : anim is over
  ANIM_LOOP,     // 01 : restart anim
  ANIM_CONDLOOP, // 02 : restart anim if testpoints are valid
  ANIM_MOVE_SPR, // 03 : alter sprite offset (compound sprites)
  ANIM_SET_SPR,  // 04 : modify frame
  ANIM_MOVE_OBJECT,//05 : alter GOB position (or delay until has moved)
  ANIM_DELAY,    // 06 : wait for X steps
  ANIM_CHECK,    // 07 : run testpoints and says "fail" if they're wrong.
  /** more commands could alter ALPHA, rotation, etc. **/
};

/** context for evaluating expressions */
struct GobCollision {
  const GobArea *area;
  GameObject *gob;
  s16 *data;
};

class Mallocator {
public:
  Mallocator() {}
  ~Mallocator() {}
  void* place(size_t sz) {
    return malloc(sz);
  }
  void release(void* p) {
    return free(p);
  }
};

class GobTank {
  std::vector<void*> chunks;
  unsigned offset;
  unsigned lost;
  static const size_t CK=65536;
public:
  GobTank() : chunks(), offset(0), lost(0) {
    chunks.push_back(malloc(CK));
    chunks.reserve(16);
  }
  ~GobTank() { flush(); }
  void release(void* p) {
    // tank only release memory when you're done with the whole tank.
  }
  void* place(size_t sz);
  void flush();
};

#define INTANK() void* operator new(std::size_t sz) { return tank->place(sz); } \
  void operator delete(void *ptr) { if (ptr) tank->release(ptr); }


/** an animation. Parses and provide commands for the suited type of
 *   object.
 */
class GobAnim : iReport, iAnimUser, UsingTank, UsingParsing, private UsingSprites {
  NOCOPY(GobAnim);
  void *ready;
  unsigned *parsed;
  const SpritePage** pages; // owned by SpriteSet
  const SpritePage* pg;
  unsigned boxdata;
  u16 nblimbs,mask;
  bool ownready, islooping, translated;
#ifdef GLOBAL_DEBUGGER
  friend GLOBAL_DEBUGGER;
#endif
  
  static u8 oamstack[128];
  static unsigned oamstop;
public:
  INTANK();
  void safecheck(unsigned** it);

  /** tells the desired type of object (SimpleGob, CompoundGob or something else) */
  enum AnimMode { SIMPLE, COMPOUND, OPENGL };
  enum RenderMode { NORMAL, BLENDED, WINDOWED, BITMAP } mode;
  unsigned getnlimbs() const { return nblimbs; }
  static void recycle(u8 oam);
  static void reset() {
    oamstop=0;
  }

  GobAnim();

  void useWindow() {
    mode = WINDOWED;
  }

  u16 getOamFlags() const {
    return (u16)(mode << 10);
  }
  
  inline enum AnimMode getmode() const {
    return (nblimbs==1) ? GobAnim::SIMPLE : GobAnim::COMPOUND;
  }

  /** creates an animation for a flat gob.
      so we don't wonder who should own the automatically created structure.
  */
  GobAnim(const SpritePage *p);
  
  /** this method is only valid on structured gob. */
  const SpritePage** getpages() const {
    return pages;
  }
  
  const SpritePage* getpage() const {
    if (pages)
      return pages[0];
    else return pg;
  }

  /** called when instanciating GameObjects: fill in OAMs.
   */
  bool prepare(u8 oams[], uint* nb, GameObject::CAST cs) const;
  
  bool prepare(u16 &oam) const;

  void setmask(unsigned m) { mask = m&0xffff; }
  unsigned getmask() const { return mask; }

  const char* parse(InputReader *input);

  unsigned getbox() const {
    return boxdata;
  }

  /** converts parsed list of commands, making sure they can be used for BlockAnim.
   *  this implies that SPRNO updates are translated to use tileno rather than
   *  pageno + blockno 
   */
  const void *translatecommands() const;

  /** converts the parsed list of commands into a packed array
   * that can be used by animation replayers. 
   */
  const void *getcommands() const;

  /** ready array is shared (and under the control of the resource
   ** manager (GobScript) for multi-limb animations.
   **/
  ~GobAnim();
  
  bool testloop() const {return islooping;}
};

// see below
class GobState;

// see GameObject.h
class GameObject;
#include "GobExpression.cxx"

/** expresses a state->state transition, indicating the predicate
 *  expression that must be met in order for the transition to take
 *  place and the action to be applied before we turn to the target
 *  state.
 */

class GobTransition : UsingTank {
  NOCOPY(GobTransition);
  const GobState *target; // which state should we branch to ?
  GobExpression<GobTank> pred;
  GobExpression<GobTank> action;
  bool breakpoint;
  void* self;
  friend class InspectorWidget;
public:
  INTANK();
  GobTransition(const GobState* st);
  
  static void safecheck(unsigned** it);

  ~GobTransition();

public:
  bool predicate(GobCollision *c, bool focused=false);
  
  const GobState *outstate(GobCollision *c);
  bool parse_pred(char *expr);
  bool parse_action(char *expr);
  bool use_anim(GobAnim *an);
};



void cleartransitions(std::vector<GobTransition*> &vt, unsigned own=~0U);

struct GobStateBuilding;

extern GobTransition* noTransitions;

/** the GobState is one atom of game entities (Gob) behaviour.
 *  it can be associated a GobAnim to play, feature transitions
 *  towards other GobStates to describe the full behaviour as a 
 *  state machine, and capture different properties that are
 *  state-dependent, such as hit boxes.
 */
class GobState : iReport, iAnimUser, UsingTank, UsingParsing {
  NOCOPY(GobState);
  char name[8];
  GobAnim *anim;
  iGobController *ctrl;

  typedef std::map<const std::string, iControllerFactory*> MAP;
  typedef std::pair<const std::string, iControllerFactory*> PAIR;
  static MAP controllers;
public:
  static const unsigned NAREAS = 4;
private:
  static bool controllersReady;
  GobTransition **onfail,**ondone;
  GobStateBuilding *building;

  u32 owndone, ownfail; // who's responsible for shared transitions ?
#ifdef GLOBAL_DEBUGGER
  friend GLOBAL_DEBUGGER;
#endif

  const GobState* checkself(const GobState* checkme) const {
    if (checkme==&GobState::self) return this;
    else return checkme;
  }
public:
  INTANK();
  const static GobState nil,self;

  void dumpchecks() const;
  static bool regctrl(iControllerFactory* fact);

  GobState(GobAnim *a, const char* pn=0);

  /** consider all state transitions have now been inserted.
   */
  void freeze();
  
  virtual ~GobState();
  
  inline enum GobAnim::AnimMode animMode() const {
    return anim->getmode();
  }

  void addOnfail(GobTransition* t, bool own=true);
  void addOndone(GobTransition* t, bool own=true);

  const char* parse(InputReader *input, unsigned *meds=0, unsigned medsize=0);

  void queue_controller(iGobController *ct);

  const GobState* onDoneChecks(GobCollision *c) const;
  const GobState* onFailChecks(GobCollision *c, bool focused=false) const;

  const GobState* dochecks(u16 x, u16 y, 
		    GobCollision *c, bool verbose=false) const;

  iGobController* getcontroller() const {
    return ctrl;
  }

  const void* getcommands() const {
    if (!anim) return 0;
    return anim->getcommands();
  }

  void setpname(int no) {
    sniprintf(name,8,"%i",no);
  }

  const GobAnim *getanim() const {
    return anim;
  }

  const char* pname() const {
    return name;
  }
};

/** an intermediate, abstract class for functions and members that are 
 **  common across all the implementations of GOBs.
 **/
class CommonGob : public GameObject {
  NOCOPY(CommonGob);
public:
  virtual bool switchstate(const GobState* st)=0;
  static const unsigned FIRSTPASSIVE=16,
    FIRSTACTIVE=1;

protected:
  iGobController* ctrl;
  const GobState* state; // the state we're in ...
  bool reload_anim;     // for use by the sub-class to know whether we changed
  bool forcechecks;
  iThought lastth;
  u16 oam;        // used by InspectorWidget to report on Gob.
  u8 areamask; //!< one bit per area. Bit set means area ignored.
  
  CommonGob(CAST _cast, GobState *init, int gno=-1);

  bool _setstate(const GobState* st);
  static const GobState* onEventChecks(GobCollision *c, GobTransition **onevent);

  virtual void setxy(world_pixel_t x, world_pixel_t y)=0;
  virtual Animator::donecode play()=0;

  Animator::donecode gobDoChecks();

  void gobRunControllers(GobCollision *c);

  const GobAnim* gobCollisions();
};
#endif
