#include <nds.h>
#include "Animator.h"
#include <stdio.h>
namespace {
  void doubleDeleteError() {
    iprintf("double delete !_?");
  }
}

/** The class that invokes play() methods of Animator instances under the hood.
 *  it has the lists of Animator, manage their registering-related state by
 *  putting them in the right list.
 *  Only Animators should know this exists.
 */
class AnimationScheduler {
  friend AnimationTester;
  Animator *todo;    //!< the things we will play at the next round
  /** the things registered while playing. 
   * avoid updates during the animation itself...
   */
  Animator* pending;
  /** the objects wanting to be deleted, but still existing for one more round.
   *  this helps fixing object-to-object links for dynamic Animator instances.
   */
  Animator* deleteme;
  bool animating;
public:
  AnimationScheduler() :
    todo(0),
    pending(0), deleteme(0), animating(false)
  {
  }

  bool IsAnimating() {
    return animating;
  }
  
  /** inserts _a_ before the first todo Animtaor item that has delay
   *  longer or equal to a. Only checks whether a is already in the
   *  list when runchecks is set to true.
   */
  bool Register(Animator* a, bool runchecks) {
    unsigned nb=0;
    if (animating) {
      pending=a->prepend(pending);
      return false;
    }
    if (runchecks)  {
      for (Animator *q=todo;q;q=q->next()) {
        if (q==a) {
          return false;
        }
        else nb++;
      }
    }
    if (a) todo=a->queue(todo);
    return true;
  }

  void Delete(Animator *a) {
    if (a->deleted())
      doubleDeleteError();
    
    if (a->deleteme() && deleteme) {
      deleteme=deleteme->dequeue(a);
      a->flags |= Animator::_deleted;
      return;
    } 
    if (todo) {
      todo=todo->dequeue(a);
    }
    a->flags |= Animator::_deleted;
  }
  
  int Count(bool show) {
    Animator *a = todo;
    int count=0;
    if (show) iprintf("[");
    while (a) {
      count++;
      if (show) iprintf("%s,",a->pname);
      a=a->next();
    }
    if (show)   iprintf("]");
    a=pending;
    if (show)   iprintf("(");
    while (a) {
      count++;
      if (show)     iprintf("%s,",a->pname);
      a=a->next();
    }
    if (show)  iprintf(")");
    return count;
  }

  /** reganim adds items *before* the first item with identical
   *   wait delay. @delayed list contains what had to be anticipatively
   *   executed to meet attachment constraints. We move them 
   *   ahead of the todo list to have them processed first next time.
   *  filled in Animate(), flushed in ProcessPending()
   */
  Animator *delayed;
  /** Animators identified in Animate() that need actual deletion in ProcessPending()
   */
  Animator *deletenow;

  /** process the lists for one round.
   */
  void Animate() {
    unsigned offset=0;
    animating=true;
    Animator *next=0;
    deletenow = deleteme;
    deleteme=0;
    delayed = 0;
    for (Animator* a=todo; a; a=next) {
      next=a->next();
      if (a->isdone()) {
        if (a->deleteme())
          deleteme=a->prepend(deleteme);
        else
          delayed=a->prepend(delayed);
        todo=next;
        continue; // it's no longer in the list!
      }

      if (a->tick()) {
        offset+=5;
        switch (a->play()) {
        case Animator::QUEUE: 
          a->flags |= Animator::_isdone;
          pending=a->prepend(pending);
          break;
        case Animator::DISMISS: 
          break;
        case Animator::DELETE:
          deleteme=a->prepend(deleteme);
          // attached GOB have one frame to detach.
          break;
        }
        todo=next;
      }
      else break;
    }
    animating=false;
  }

  void ProcessPending() {
    while (pending) {
      Animator *a=pending;
      a->flags &= ~Animator::_isdone;
      pending=pending->next();
      a->regAnim();
    }
    while(delayed) {
      Animator *a=delayed;
      a->flags &= ~Animator::_isdone; /** Animator::queue could automate that (?) */
      delayed=delayed->next();
      a->regAnim();
    }
    while(deletenow) {
      Animator *a=deletenow;
      deletenow=a->next();
      delete a;
    }
  }

  void ClearAnimators() {
    todo = ClearAnimatorList(todo);
    deleteme = ClearAnimatorList(deleteme);
    pending = ClearAnimatorList(pending);
  }
  
private:
  Animator* ClearAnimatorList(Animator *lst) {
    Animator *start = lst, *prev = 0;
    while (lst) {
      Animator *del = lst;
      lst = lst->next();
      if (del->flags & Animator::_autoclear) {
        if (prev) prev->nxt = del->next();
        else start = del->next();
        delete del;
      } else {
        prev = del;
      }
    }
    return start;
  }
} AnimScheduler;

Animator::~Animator() 
{ 
  AnimScheduler.Delete(this);
}

void Animator::Animate() {
  AnimScheduler.Animate();
}

void Animator::ProcessPending() {
  AnimScheduler.ProcessPending();
}

void Animator::regAnim(bool runchecks) {
  AnimScheduler.Register(this, runchecks);
}

bool Animator::IsAnimating() {
  return AnimScheduler.IsAnimating();
}

void Animator::ClearAll() {
  AnimScheduler.ClearAnimators();
}
