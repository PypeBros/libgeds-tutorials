#include "nds.h"
#include "interfaces.h"

/** reads the buttons & dpad state and make them available in one of the target gob variables
 */
class DpadController : public iGobController {
  NOCOPY(DpadController);
  /** some keys are only mentioned "pressed" for a few frames after they are actually changed
   *  from 'released' to 'pressed' state, to allow e.g. that your character doesn't keep
   *  jumping. timeout indicates how many frames before the button is considered pressed
   *  for too long.
   */
  static int timeout[32];

  /** each state can define which buttons/direction must use timeout array */
  unsigned timemask;
  static int delay;
  /** knowing which button/direction were pressed at the previous frame is critical to 
   *  know which be considered a 'new' keypress.
   *  WARNING: this code assumes only one GameObject uses a DpadController
   */
  static int prevkeys;
public:
  DpadController(const iControllerFactory *icf, unsigned m) : iGobController(icf), timemask(m) {}
  virtual ~DpadController() {}
  virtual iThought think(s16 gob[8], GameObject *g) {
    iThought me = NONE;
    int prevdirs = gob[GOB_DIRS];
    int newkeys  = 0;
    int kh = (keysHeld()|keysDown()) & ~(KEY_START|KEY_SELECT);

    for (unsigned i=0,j=1 ; i<31 ; i++, j=j<<1) {
      if ((kh&j)) { 
	if (!(prevkeys & j)) timeout[i]=delay;
	else if ((timemask & j) && timeout[i]>0) timeout[i]--;
      } else timeout[i]=0;
      if (timeout[i]) newkeys|=j;
    }
    gob[GOB_DIRS]=newkeys;
    prevkeys=kh;
    return combine(me, gob, g);
  }
};

int DpadController::delay=6;
int DpadController::prevkeys=0;
int DpadController::timeout[32];

class GobDpadFactory : public iControllerFactory {
  NOCOPY(GobDpadFactory);
public:
  GobDpadFactory(const std::string str) {
    name=str;
  }
  virtual iGobController* create(char* args) {
    unsigned mask=KEY_DPAD|KEY_R;
    siscanf(args,"%x",&mask);
    return new DpadController(this, ~mask);
  }
  virtual ~GobDpadFactory() {}
};

namespace {
  GobDpadFactory dpad("dpad");
};

