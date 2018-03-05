#include "Resources.h"
#include <nds.h>
#include "UsingSprites.h"
/** captures the current state of resource allocation by the GuiEngine into to[]
 *   so that it can later be restored at once.
 *  if regsto is not null, it must point towards an array of SAVEREGS u16 where
 *   the video setup configuration (per-layer and general) will be saved.
 */
void Resources::snap(Resources &to) {
    for (uint i=0;i<UsingResources::RES_SPECIALTYPES;i++) to.rescount[i]=rescount[i];
}

void Resources::restore(Resources &from) {
  for (uint i=0;i<UsingResources::RES_NBTYPES;i++) rescount[i]=from.rescount[i];
}

Resources::Resources() {
  for (unsigned i=0;i<UsingResources::RES_NBTYPES;i++) { rescount[i]=0; }
}

unsigned Resources::allocate(restype rt, unsigned nb) {
  unsigned n=rescount[rt];
  rescount[rt]+=nb;
#ifdef GLOBAL_DEBUGGER
  if (rt==RES_OAM && n >=128) {
    throw EngineError("Over-Allocation of OAMs");
  }
#endif
  if (rt==RES_OAM_SUB) n+=128;
  if (rt==RES_ROT_SUB) n+=32;
  return n;
}

unsigned Resources::get(restype rt) {
  return rescount[rt];
}

void Resources::reset(restype rt) {
  rescount[rt] = 0;
}

Resources RealResources;
Resources *UsingResources::gResources = &RealResources;
