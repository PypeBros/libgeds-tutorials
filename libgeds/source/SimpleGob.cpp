#include <sys/types.h>
#include <nds.h>
#include "StateMachine.h"

/**
 * A SimpleGob is a GameObject that has only one OAM, but is operated
 * through a state machine.
 */

class SimpleGob : public CommonGob, iAnimUser, private UsingSprites {
  NOCOPY(SimpleGob);
  /*const AnimCommand */unsigned* reading;
  const SpritePage *pg; // later useful for setOAM ;->
  friend class InspectorWidget;

  // vGobArea *areas;
  blockno_t frame;
  unsigned dly;
  unsigned anim;
  s8 origin[2];

  inline bool setstate(const GobState* st) {
    bool r=CommonGob::_setstate(st);
    if (reload_anim) loadAnim();
    return r;
  }


public:
  /** create a static sprite at fixed coordinates. */
  SimpleGob(CAST _cast, GobState *init, int gno=-1) :
    CommonGob(_cast,init,gno), 
    reading(0),pg(0),frame(INVALID_BLOCKNO),dly(0),anim(0), origin()
  {
    const GobAnim *ga = init->getanim();
    if (ga) {
      ga->prepare(oam);
      pg =ga->getpage();
    } else iprintf(">_< no anim on init state\n");
    if (pg) {
      unsigned bd = ga->getbox();
      if (bd>>16) {
	s8 bx, by;
	u16tos8(bd,&bx,&by);
	setbbox(bx,by);
      } else
	setbbox(pg->getWidth()*8,pg->getHeight()*8);
    } else iprintf(">_< no page on init state\n");
    setstate(init); // also sets controller and animation sequence.
  }

  virtual ~SimpleGob() {
    // delanim happens in ~Animator.
    sprites[oam].attribute[0]=ATTR0_DISABLED;
    GobAnim::recycle(oam);
  }



  virtual void dump(const char* msg=0) {
  }


  inline bool loadAnim() {
    reading=(/*const AnimCommand*/unsigned*) state->getcommands();
    { const GobAnim *ga=state->getanim();
      if (ga) pg=ga->getpage();
      else pg=0;
    }
    dly=0;
    reload_anim=false;
    return true;
  }

  /** the external interface to setstate(). */
  virtual bool switchstate(const GobState* st) {
    return setstate(st);
  }


  /* try to move along the ground */
  inline bool trywalk(int dx) {
    return false;
  }

  /* try to move by (dx,dy), 
   * - either */ 


  inline bool trymove(int dx, int dy) {
    // const GobState* curr=state;
    forcechecks=false;

    {
      x+=dx;
      y+=dy;
      return true;
    }
    
  }

  virtual void setxy(world_pixel_t x, world_pixel_t y) {
    this->x=x<<8; this->y=y<<8;
//    pg->changeOAM(sprites+oam,x-cam->getx(),y-cam->gety(),frame);
  }

  void draw() {
    /** oh, btw, don't worry about performances or graphical glitches due
     *  to our "think-move-draw-repeat" approach: we don't manipulate 
     *  graphical values directly, merely a cache that will be committed
     *  to VRAM at the next blank period.
     **/
    s16 dx=(s16)(x>>8);
    s16 dy=(s16)(y>>8);
    if (dx>-64 && dx<256 && dy>-64 && dy<192) {
      pg->changeOAM(sprites+oam,dx+origin[0],dy+origin[1],frame);
      sprites[oam].attribute[2]|=
	raise?ATTR2_PRIORITY(1):ATTR2_PRIORITY(2);
    } else {
      sprites[oam].attribute[0]=
	(sprites[oam].attribute[0]&~ATTR0_NORMAL)
	|ATTR0_DISABLED;
    }
  }

  void hide() {
    sprites[oam].attribute[0]=
      (sprites[oam].attribute[0]&~ATTR0_NORMAL)
      |ATTR0_DISABLED;
  }    
  
#define ANIM(x, y) ((x*16)|y)
  
  // we *need* repeated invocation of this method to stay in place!
  // if you want a sprite that tracks camera movement, move sprite
  // along the world, and adjust camera to sprite coordinates.
  virtual Animator::donecode play() {
    GobCollision *c = 0;
    Animator::donecode done=gobDoChecks();
    if (state==&GobState::nil) hide();
    if (done!=Animator::QUEUE) return done;

    gobRunControllers(c);
    if (reload_anim) loadAnim();

    if (paused>1) {
      //      if (focus) printf("paused: %i\n",paused);
      if (focus) paused=paused-1;
    }

    if (dly) {
      dly--;
    } else {
      /* watch out : if we have no delay at all in the anim,
       *  we could get caught in endless evaluation of the loop.
       *  we should ensure that we won't execute two 'loop' commands
       *  for a single run.
       */
      bool stop=false; bool looping=false; uint loops=0;
      while(reading && !stop) {
	looping=false;
	if (loops>4) {
	  iprintf("oops? endless looping");
	  break;
	}
	if (focus) { 
	  // iprintf(CON_GOTO(1,9) "[gob#%02x:%02x:%i]",oam,reading->type,
	  //	  reading - state->getcommands());
	}
	if (command(*reading)==C_EDITOR) continue;
	switch((*reading)>>24) {
	case ANIM(C_CONTROL,I_DONE):
	  if (state && !setstate(state->onDoneChecks(c)))
	    reading=0; 
	  stop=true;   // to avoid deadloop on st{done}->st{done}
	  break;
	case ANIM(C_CONTROL,I_LOOP): 
	  reading=(unsigned*) state->getcommands(); 
	  looping=true;
	  break;
	  
	case ANIM(C_UPDATE,I_SPRNO):
	  frame=(blockno_t)(*reading&0xff);
	  /* bits 14 and 15 of the sprite ID are interpreted as H/V flip */
	  sprites[oam].attribute[1]=
	    (sprites[oam].attribute[1]&~(ATTR1_FLIP_X|ATTR1_FLIP_Y))
	    |((*reading&0xC000)>>2);
	  sprites[oam].attribute[2]&=0x0fff; // clear palette slot
	  sprites[oam].attribute[2]|=(((*reading)&0x3C00)<<2);

	  break;
	case ANIM(C_UPDATE,I_MOVETO): {
	  /** bits 0 and 1 in mspr.id are used to indicate whether we should
	   *  ignore dx and dy in the movement. If that's the case, we just 
	   *  use accumulated STEPX or STEPY instead.
	   */
	  int uses=object(*reading);
	  int dx;// = (reading->_.mspr.id&1)?(reading->_.mspr.dx<<8):cdata[STEPX];
	  int dy;// = (reading->_.mspr.id&2)?(reading->_.mspr.dy<<8):cdata[STEPY];
	  u16tos8(*reading,&dx,&dy);
	  if (!trymove((uses&1)?dx<<8:cdata[STEPX], (uses&2)?dy<<8:cdata[STEPY])) {
	    looping=true;
	    stop=true;
	  }
	  break;
	}
	case ANIM(C_UPDATE,I_COORDS):
	  u16tos8(*reading,origin,origin+1);
	  break;
	default:
	  iprintf("%x not supported",*reading);
	  break;
	case ANIM(C_CONTROL,I_DELAY):
	  dly=*reading&0xff;
	  break;
	}
	if (reading && !looping) reading++;
	if (dly) break;
      } 
    }
    // HOOK: gobCollisions();
    
    if (state == &GobState::nil) {
      /* object has been destroyed. */
      hide();
      flags |= Animator::_deleteme;
      iprintf("[-gob%i] ", gobno);
      return Animator::DELETE;
    } else {
      if (frame==256) hide();
      else draw();
      return Animator::QUEUE;
    }
  }
#undef ANIM
  void getcoords(int &_x, int &_y, int &_xs, int &_ys) {
    _x=x>>8;
    _y=y>>8;
    _xs=cdata[0]>>8;
    _ys=cdata[1]>>8;
  }
  
};

GameObject* GameObject::CreateSimpleGob(CAST _cast, GobState *init, int gno)
{
  return new SimpleGob(_cast, init, gno);
}
