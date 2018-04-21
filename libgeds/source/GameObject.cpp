#include <vector>
#include <list>
#include <sys/types.h>
#include <nds.h>
#include "SpriteSet.h"
#include "GuiEngine.h"
#include "interfaces.h"
#include "GameConstants.h"
#include "SpriteAnim.h" // for iAnimUser
#include "GameObject.h"
#include "StateMachine.h"

void* GobTank::place(size_t sz) {
    void* ret = (void*)((char*)chunks.back() + offset);
    if (offset+sz < CK) {
#ifdef TRACE_TANK_ALLOC
      TRACE_TANK_ALLOC(ret);
#endif
      if (sz & 3) sz = (sz + 4) & ~3;
      offset += sz;
      * (char*) ret = 0xca;
      return ret;
    } else {
      if (sz > CK / 2) throw std::bad_alloc();
      lost += (CK - offset);
      offset = 0;
      chunks.push_back(malloc(CK));
      return place(sz);
    }
}

void GobTank::flush() {
    iprintf("loss: %i / %iK\n",lost, (CK/1024)*chunks.size());
    for (uint i=0; i<chunks.size(); i++) {
      free(chunks[i]);
    }
    chunks.clear();
    lost=0;
    offset=CK;
}

void GobAnim::recycle(u8 oam) {
    oamstack[oamstop++]=oam;
}

GobAnim::GobAnim() : ready(0), parsed(0), pages(0), pg(0),boxdata(0x1010),
	      nblimbs(0),mask(0),ownready(false),islooping(false),
	      translated(false), mode(NORMAL) {
}

GobAnim::GobAnim(const SpritePage *p) : 
    ready(0), parsed(0), pages(0), pg(p), boxdata(0x1010), 
    nblimbs(1), mask(0), ownready(false), islooping(false),
    translated(false), mode(NORMAL) {
    if (!p) throw iScriptException("invalid SpritePage in creating GobAnim");
}

  // called when instanciating GameObjects: fill in OAMs.
bool GobAnim::prepare(u8 oams[], uint* nb, GameObject::CAST cs) const {
    int list=cs==GameObject::HERO?1:2;
    if (pages) {
      for (uint i=nblimbs; i>0; i--) {
	if (pages[i-1]==0) throw iScriptException("page undef");
	unsigned oam;
	if (oamstop)
	  oam = oamstack[--oamstop];
	else 
	  oam=gResources->allocate(RES_OAM);
	dprint("allocated oam%i with page@%p\n",oam,pages[i-1]);
	oams[i-1]=oam;
	(pages)[i-1]->setupOAM(sprites+oam, getOamFlags()); // just fill in type, mode, etc. hide OAM.
      }
      // list=(list+1)&3;
      if (nb!=0) *nb=nblimbs;
      return true;
    } else {
      throw iScriptException("CompoundGob on non-Animeds anim");
      return false;
    }
}
  
bool GobAnim::prepare(u16 &oam) const {
    if (pages) {
      iprintf("anim is multi-pages and should use vector prepare\n");
      return false;
    }
    if (oamstop)
      oam = oamstack[--oamstop];
    else 
      oam = gResources->allocate(RES_OAM);

    if (!pg) throw iScriptException("anim has no page");
    pg->setupOAM(sprites+oam, getOamFlags());
    sprites[oam].attribute[0]|=ATTR0_DISABLED;
    sprites[oam].attribute[2]|=ATTR2_PRIORITY(1);
    
    return true;
}


const char* GobAnim::parse(InputReader *input) {
    std::vector<unsigned> building;
    char lb[256]="";
    char *l=0;
    do {
      if (!input->readline(lb,256)) break;
      if (!(l=UsingParsing::content(lb))) continue;

      { 
	unsigned frame,delay,part;
	int dx,dy;
	char star[4];
	step("[%20s]",l);
	if (siscanf(l,"bbox %i %i %i %i",&dx,&dy,&delay,&part)==4) {
	  boxdata=((delay&255)<<8)|(part&255);
	  building.push_back(encode(C_UPDATE,I_COORDS,0,s8tou16(dx,dy)));
	  continue;
	}
	/* spr%u %x (%i %i) -- combo ANIM_SET_SPR/ANIM_MOVE_SPR */
	if (siscanf(l,"spr%u %x",&part,&frame)==2) {
	  building.push_back(encode(C_UPDATE,I_SPRNO,part,frame));
	  continue;
	}
	if (siscanf(l,"delay %u",&delay)==1) {
	  building.push_back(encode(C_CONTROL,I_DELAY,0,delay));
	  continue;
	}
	if (siscanf(l,"meta %x",&delay)==1) {
	  building.push_back(encode(C_UPDATE, I_PROPS,0,delay));
	  continue;
	}
	if (siscanf(l,"move %i %i",&dx,&dy)==2) {
	  building.push_back(encode(C_UPDATE,I_MOVETO,0xf3,s8tou16(dx,dy)));
	  continue;
	}
	if (siscanf(l,"move %i %1[*]",&dx,star)==2) {
	  building.push_back(encode(C_UPDATE,I_MOVETO,0xf1,s8tou16(dx,0)));
	  continue;
	}
	if (siscanf(l,"move %1[*] %i",star,&dy)==2) {
	  building.push_back(encode(C_UPDATE,I_MOVETO,0xf2,s8tou16(0,dy)));
	  continue;
	}
	if (strncmp(l,"clear",5)==0) {
	  building.push_back(encode(C_UPDATE,I_SPRNO,0,0xffff));
	  continue;
	}
	if (strncmp(l,"loop",4)==0) {
	  building.push_back(encode(C_CONTROL,I_LOOP,0,0));
	  islooping=true;
	  continue;
	}
	if (strncmp(l,"cloop",5)==0) {
	  building.push_back(encode(C_CONTROL,I_CONDLOOP,0,0));
	  islooping=true;
	  continue;
	}
	if (strncmp(l,"check",5)==0) {
	  building.push_back(encode(C_CONTROL,I_CHECK,0,0));
	  continue;
	}
	if (strncmp(l,"done",4)==0) {
	  building.push_back(encode(C_CONTROL,I_DONE,0,0));
	  islooping=false;
	  continue;
	}
      }
    } while (!l || l[0]!='}');
    step("%i statements added to anim\n",building.size());
    parsed = (unsigned*) tank->place(sizeof(unsigned)*building.size());
    for (unsigned i=0;i<building.size();i++) {
      parsed[i]=building[i];
    }
    ownready=true;

    return "";
}

  /** converts parsed list of commands, making sure they can be used for BlockAnim.
   *  this implies that SPRNO updates are translated to use tileno rather than
   *  pageno + blockno 
   */
const void *GobAnim::translatecommands() const {
    if (ready && translated) return ready;
    if (ready && !translated) throw iScriptException("commands can't be translated");
    if (pages) throw iScriptException("cannot translate multi-pages anim");
    const_cast<GobAnim*>(this)->ready = parsed;
    for (unsigned *r = const_cast<GobAnim*>(this)->parsed; 
	 !(command(*r)==C_CONTROL && item(*r)!=I_DELAY); r++) {
      if (command(*r)==C_UPDATE && item(*r)==I_SPRNO) {
	if ((*r&0xffff) == 0xffff) {
	  *r = encode(C_UPDATE,I_SPRNO,object(*r),0);
	} else {
	  *r=encode(C_UPDATE,I_SPRNO,object(*r),
		    pg->getTile((blockno_t)(*r&0xff))|(*r&0xc000));
	}
      }
      const_cast<GobAnim*>(this)->ownready=true;
      const_cast<GobAnim*>(this)->translated=true;
    }
    return ready;
}

  /* converts the parsed list of commands into a packed array
   * that can be used by animation replayers. 
   */
const void *GobAnim::getcommands() const {
    if (translated) throw iScriptException("commands have been translated");
    if (ready) return ready;
    if (!pages) {
      /** hopefully, this does not occurs with 1-limb, Animeds anims,
	  as ready is then set in use() function.
      */
      const_cast<GobAnim*>(this)->ready = parsed;
      return ready;
    }
    throw iScriptException("anim should have pages");
}

  /** ready array is shared (and under the control of the resource
   ** manager (GobScript) for multi-limb animations.
   **/
GobAnim::~GobAnim() {
    if (ready!=0 && ownready) free(ready);
}

class GobState;
class GameObject;

GobTransition::GobTransition(const GobState* st) :
    target(st),pred(),action(),breakpoint(false),self(0) {
    self=this;
}

GobTransition::~GobTransition() {
    if (self!=this) die(__FUNCTION__,(int)self);
    self=(void*)0xdead;
    /* states and targets are still owned by the GameScript */
    /* target might potentially have been reclaimed already and pointer shouldn't be followed */
}

bool GobTransition::predicate(GobCollision *c, bool focused) {
    if (!pred.eval(c)) return false;
    return true;
}

const GobState *GobTransition::outstate(GobCollision *c) {
    action.eval(c);
    return target;
}

bool GobTransition::parse_pred(char *expr) {
    return pred.parse(expr);
}

bool GobTransition::parse_action(char *expr) {
    return action.parse(expr);
}

bool GobTransition::use_anim(GobAnim *an) {
    return true;
}

void* iGobController::operator new(std::size_t sz) { return tank->place(sz); }
void iGobController::operator delete(void* ptr) { if (ptr) tank->release(ptr); }

void cleartransitions(std::vector<GobTransition*> &vt, unsigned own) {
  for(int i=0, n=vt.size(), j=1; i<n; i++, j=j<<1)
    if (vt[i] && (own&j)) delete vt[i];
}

struct GobStateBuilding {
  std::vector<GobTransition*> onfail; // TANK me.
  std::vector<GobTransition*> ondone; // TANK me.
  GobStateBuilding() : onfail(), ondone() {}
};

void GobState::dumpchecks() const {
    /*    iprintf("flr=%x, off=%x, spd=%x, nw=%x, ",
	  chkflr, chkoff, cspeed, chknw);*/
}

bool GobState::regctrl(iControllerFactory* fact) {
    const std::string n=fact->getname();
    controllers.insert(PAIR(n,fact));
    return true;
}

GobState::GobState(GobAnim *a, const char* pn) :
    anim(a), ctrl(0),
    onfail(), ondone(),
    building(0), owndone(0)
{
    if (pn) strncpy(name, pn, 8);
    if (a==0 && strcmp(name,"nil")==0) {
      ondone = &noTransitions;
      onfail = &noTransitions;
    }
    if (a && ! controllersReady) {
      iControllerFactory::registerAll();
      controllersReady = true;
    }
}

  /** consider all state transitions have now been inserted.
   */
void GobState::freeze() {

    if (building==0) return;
    unsigned ntransitions = building->onfail.size() + building->ondone.size();
    GobTransition **t = (GobTransition**) 
      tank->place(sizeof(GobTransition*) * ntransitions);

    ondone = t;
    for (unsigned i=0, n=building->onfail.size(); i<n; i++)
      *t++ = building->onfail[i];
    *t++ = 0;
    for (unsigned i=0, n=building->ondone.size(); i<n; i++)
      *t++ = building->ondone[i];
    *t++ = 0;
    delete building;
    building = 0;
}
  
GobState::~GobState() {
    /* the anim can be shared by several states. It remains under
     * the control of the gamescript and doesn't require explicit
     * deletion here.
     */
    /** ALL YOUR STUFF ARE BELONG TO TANK */
}

void GobState::addOnfail(GobTransition* t, bool own) {
    if (own) ownfail|=(1<<building->onfail.size());
    building->onfail.push_back(t);    
}

void GobState::addOndone(GobTransition* t, bool own) {
    if (own) owndone|=(1<<building->ondone.size());
    building->ondone.push_back(t);    
}

void GobState::queue_controller(iGobController *ct) {
    if (ctrl) ctrl->enqueue(ct);
    else ctrl=ct;
}

const char* GobState::parse(InputReader *input, unsigned *meds, unsigned medsize) {
    char lb[256]="";
    char *l=0;
    building = new GobStateBuilding();
    
    do {
      if (!input->readline(lb,256)) break;
      if (!(l=UsingParsing::content(lb))) continue;

      char ctrlname[16]=""; char args[32]="";
      if (siscanf(l,"using %15[a-z] (%31[^)])",ctrlname,args)>=1) {
	std::string name(ctrlname);
	iControllerFactory *factory;
	step("checking '%s' amongst %i\n", name.c_str(), controllers.size());
	if ((factory = controllers[name])) {
	  step("got it (%p).\n",factory);
	  queue_controller(factory->create(args));
	} else throw iScriptException("no '%s' controller registered.\n", ctrlname);
      }

    } while (!l || l[0]!='}');
    return "";
}


const GobState* GobState::onDoneChecks(GobCollision *c) const {
    for (unsigned i=0;ondone[i] ;i++)
      if (ondone[i]->predicate(c)) return checkself(ondone[i]->outstate(c));
    return 0;
}

const GobState* GobState::onFailChecks(GobCollision *c, bool focused) const {
    for (unsigned i = 0; onfail[i]; i++)
      if (onfail[i]->predicate(c, focused)) return checkself(onfail[i]->outstate(c));
    return 0;
}

const GobState* GobState::dochecks(u16 x, u16 y,
				   GobCollision *c, bool verbose) const
{
    return onFailChecks(c);
}


bool iControllerFactory::registerAll() {
  for (iControllerFactory* p = allUnregistered; p; p = p->nextUnregistered)
    if (! GobState::regctrl(p)) return false;
  return true;
}
iControllerFactory* iControllerFactory::allUnregistered = 0;
bool GobState::controllersReady = false;

GameObject::GameObject(CAST c, int gno, const char *pname) : 
  iDebugable(0, pname), self(), cdata(), hotx(0), hoty(0),
  x(0), y(0), wbox(0), hbox(0), gobno(gno),
  focus(false),  raise(false), mayfreeze(true), 
  cast(c)
{
  memset(cdata,0,sizeof(cdata));
  if (c!=DONTLIST) 
    self=gobs[c].insert(gobs[c].end(), this);
}

GameObject::~GameObject() {
  if (cast != DONTLIST) gobs[cast].erase(self);
}

void GameObject::setbbox(int w, int h) {
  wbox = w; hbox = h;
  hotx = w/2;
  hoty = h-1;
}

//#define DBG(x) if (focus) iprintf(x)
#define DBG(x) 

void GameObject::clearDynGobs() {
  while(!gobs[DYNAMIC].empty()) {
    GameObject *g = gobs[DYNAMIC].front();
    delete g;
  }
  while(!gobs[HERO].empty()) {
    GameObject *g = gobs[HERO].front();
    delete g;
  }
  while(!gobs[EVIL].empty()) {
    GameObject *g = gobs[EVIL].front(); // crash ?
    delete g;
  }
}

CommonGob::CommonGob(CAST _cast, GobState *init, int gno) :
    GameObject(_cast, gno, (char*)((_cast==HERO)?"hero":"sgob")), 
    ctrl(0),state(0), 
    reload_anim(false),forcechecks(false), lastth(INIT),oam(0),areamask(0)
{
}

bool CommonGob::_setstate(const GobState* st) {
    if (!st) return false;
    if (state!=st) {
      if (st==&GobState::self) st=state; // -> self doesn't reload the animation.
      else reload_anim=true;
      ctrl =st->getcontroller();
      state=st;
    } else reload_anim=true;
    return true;
}

Animator::donecode CommonGob::gobDoChecks() {
    if (!running()) {
      // iprintf("%s stopped",pname);
      return Animator::DISMISS;
    }
    if (state == &GobState::nil) {
      /* object has been destroyed. Don't think, die.*/
      //      hide();
      flags |= _deleteme;
      return Animator::DELETE;
    }
    flags |= _isdone; // so we're sure we won't get called again.
    return Animator::QUEUE;
}

void CommonGob::gobRunControllers(GobCollision *c) {
    iThought th=NONE;
    if (ctrl) th = ctrl->think(cdata,this);
    lastth=th;

    switch(th) {
    case INIT:
      break;
    case NONE:
      if (focus && debugging) iprintf(CON_GOTO(0,24) "  ");
      {
	x += cdata[0]; y += cdata[1];
      }
      break;
    case FAIL:
      {
	x += cdata[0]; y += cdata[1];
      }
      if (_setstate(state->dochecks(px(), py(), c, debugging&&focus))) { 
	// hook : state transition occured
      } else { 
	// hook : state transition cancelled.
      }
      break;
    case EVENT:
      break;
    }
}


Mallocator withMalloc;
template<>
Mallocator* GobExpression<Mallocator>::tank = &withMalloc;
Mallocator* UsingMalloc::tank = &withMalloc;

void UsingTank::settank(GobTank *tk) {
  tank=tk;
}

GameObject::GobList GameObject::gobs[NBLISTS];
const GobState GobState::nil(0,"nil");
const GobState GobState::self(0,"self");
GobState::MAP GobState::controllers;

int GameObject::paused = 0;
bool GameObject::debugging=false;
unsigned GobAnim::oamstop=0;
u8 GobAnim::oamstack[128];

GobTransition **GameObject::doevents;
GobTransition* noTransitions=0;

const char* GameObject::force="forced";
GobTank* UsingTank::tank=0;
