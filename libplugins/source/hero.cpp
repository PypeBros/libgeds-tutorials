#include "nds.h"
#include "interfaces.h"

class VmomentumController : public iGobController {
  NOCOPY(VmomentumController);
  int maxspeed;  // absolute maximum speed
  int increment; // absolute speed increment.
public:
  VmomentumController(const iControllerFactory *icf, int mx, int ic) : 
    iGobController(icf), maxspeed(mx), increment(ic) {}
  ~VmomentumController() {}
  virtual iThought think(s16 gob[8], GameObject *g) {
    iThought me = NONE;
    int prevspeed = gob[GOB_XSPEED];
    if (gob[GOB_DIRS]&KEY_UP && gob[GOB_YSPEED]>-maxspeed)
      gob[GOB_YSPEED]-=increment;
    if (gob[GOB_DIRS]&KEY_DOWN && gob[GOB_YSPEED]<maxspeed)
      gob[GOB_YSPEED]+=increment;
    return combine(me, gob, g);
  }
};

class HmomentumController : public iGobController {
  NOCOPY(HmomentumController);
  int maxspeed;  // absolute maximum speed
  int increment; // absolute speed increment.
public:
  HmomentumController(const iControllerFactory *icf, int mx, int ic) : 
    iGobController(icf), maxspeed(mx), increment(ic) {}
  ~HmomentumController() {}
  virtual iThought think(s16 gob[8], GameObject *g) {
    iThought me = NONE;
    int prevspeed = gob[GOB_XSPEED];
    if (gob[GOB_DIRS]&KEY_LEFT && gob[GOB_XSPEED]>-maxspeed)
      gob[GOB_XSPEED]-=increment;
    if (gob[GOB_DIRS]&KEY_RIGHT && gob[GOB_XSPEED]<maxspeed)
      gob[GOB_XSPEED]+=increment;
    return combine(me, gob, g);
  }
};

class GobMomentumFactory : public iControllerFactory {
public:
  GobMomentumFactory(const std::string str) {
    name=str;
  }
  virtual iGobController* create(char* args) {
    int ms=1024, inc=32;
    if (siscanf(args,"y to %i by %i",&ms,&inc))
      return new VmomentumController(this, ms,inc);
    else if (siscanf(args,"x to %i by %i",&ms,&inc))
      return new HmomentumController(this, ms,inc);
    die(__FILE__,__LINE__);
    return 0;
  }
  virtual ~GobMomentumFactory() {}
};

class GobFreemoveController : public iGobController {
  NOCOPY(GobFreemoveController);
  uint thru;

public:
  GobFreemoveController(const iControllerFactory *icf, uint thr) : 
    iGobController(icf),thru(thr)  {}
  virtual iThought think(s16 gob[8], GameObject *g) {
    iThought th = NONE;
    int dx = gob[GOB_XSPEED];
    int dy = gob[GOB_YSPEED];

    bool couldmove=true;
    if (thru) couldmove=g->cando(dx,dy,thru);
    else g->setraise(); // GOBs that move through everything show above everything.
    if (!couldmove) gob[GOB_YSPEED]=0;
    else return combine(th,gob,g);

    if (!g->cando(dx,0,F_FALLTHRU)) {
      gob[GOB_XSPEED]=0;
      if (dy && g->cando(0,dy,F_FALLTHRU))
	gob[GOB_YSPEED]=dy;
      else {
	gob[GOB_YSPEED]=0;
	th=FAIL; 
      }
    } 
    return combine(th,gob,g);
  }
  virtual ~GobFreemoveController() {}
};


class GobFreemoveFactory : public iControllerFactory {
public:
  GobFreemoveFactory(const std::string str) {
    name=str;
  }
  virtual iGobController* create(char *arg) {
    uint thru; 
    if (!arg || siscanf(arg,"%x",&thru)<1) thru=F_FALLTHRU;
    return new GobFreemoveController(this,thru);
  }
  virtual ~GobFreemoveFactory() {}
};

namespace {
  GobMomentumFactory momentum("momentum");
  GobFreemoveFactory freemove("freemove");
}

