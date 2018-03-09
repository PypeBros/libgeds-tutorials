#ifndef __ANIMATOR__
#define __ANIMATOR__

#include "nocopy.h"
class AnimationScheduler;
class Animator;

class AnimationTester {
  void CheckAnimatorsList(Animator* list = 0, const char* name = "");
public:
  enum which_list {
    NONE=0,
    TODO=1,
    PENDING=2,
    DELETING=4,
    ALL=TODO|PENDING|DELETING
  };
  virtual ~AnimationTester() {}
  Animator* FindAnimator(const char *pname);    
  Animator* AnimatorFromName(const char* name);
  void CheckAnimators(which_list which = ALL);
};

class Animator {
  friend AnimationScheduler;
  friend AnimationTester;
  NOCOPY(Animator);
  Animator *nxt;
 protected:
  unsigned delay;
  unsigned flags;
  /** set when the animator should execute on play() */
  static const unsigned _running = 1;
  /** set when the engine has executed play() this frame,
   *  reset when the animator is moved from pending to todo again
   */
  static const unsigned _isdone = 2;
  /** indicates the object should be deleted at the next round,
   *  letting attached object some time to detach.
   */
  static const unsigned _deleteme = 4;
  static const unsigned _deleted = 8;
  /** indicates that the object should be destroyed on ClearAnimatorList.
   * (this is for non-gob objects that depend on GameScript to execute properly)
   */
  static const unsigned _autoclear = 16;
  bool running() { return (flags & _running) == _running; }
  bool isdone() { return (flags & _isdone) == _isdone; }
  bool deleteme() { return (flags & _deleteme) == _deleteme; }
  bool deleted() { return (flags & _deleted) == _deleted; }
 public:
  const char* pname;
  Animator(unsigned d, const char* pn="----", unsigned flags=0) :
    nxt(0),delay(d),flags(flags),
    pname(pn) {}
  Animator():
    nxt(0),delay(1),flags(0),
    pname("----") {}
  typedef enum {DISMISS, QUEUE, DELETE} donecode;
  virtual donecode play(void)=0;

  /** indicates that play() should be called periodically.
   *  (only used if you use it in play())
   */
  Animator* run() {
    flags |= _running;
    return this;
  }

  void stop() {
    flags &= ~_running;
  }
  
  /* remove an entry from the queue (if present) */
  Animator* dequeue(Animator *x)
  {
    if (x==this) return nxt;
    if (nxt) nxt=nxt->dequeue(x);
    return this;
  }

  /* just chain. */
  Animator* prepend(Animator *q)
  {
    nxt=q;
    return this;
  }

  /* insert yourself in the queue at proper place */
  Animator* queue(Animator *q) {
    Animator *p=0; /* p precedes q, right? */
    Animator *full=q;
    while (q && q->delay < delay) {
      delay-=q->delay;
      p=q;
      q=q->nxt;
    }
    /* now we're somewhere in the queue where we should insert */
    nxt=q;
    if (p) {
      p->nxt=this;
      return full;
    } else return this;
  }

  /** count one time, tells whether it is time to call play()
   */
  inline bool tick(void) {
    if (delay) {
      delay--;
      return false;
    } else return true;
  }

  Animator* next() { return nxt;}
  virtual ~Animator();
  /** run the play() method on all the instances */
  static void Animate();
  /** do the cleanup after Animate() */
  static void ProcessPending();
  /** are we calling in the context of a play() during Animate() ? */
  static bool IsAnimating();
  static void ClearAll();
  /** add this Animator in the scheduler's internal lists to be played on Animate() */
  void regAnim(bool runchecks=false);
};
#endif
