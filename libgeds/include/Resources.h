/* -*- c++ -*- */
#ifndef __RESOURCES_H
#define __RESOURCES_H
#include <sys/types.h>
class Resources;
class UsingResources {
public:
  typedef enum {RES_OAM,     //!< allocates one OAM slot for the main screen (value 0-127)
		RES_OAM_SUB, //!< OAM slot for sub-screen (value 128-255)
		RES_SPRITETILE, //!< allocates one tile (64 bytes)
		RES_SPRITETILE_SUB,
		RES_BGTILE,
		RES_BGTILE_SUB,
		RES_ROT,     //!< allocates an affine matrix for main screen (val 0-31)
		RES_ROT_SUB, //!< affine matrix for the sub-screen (value 32-63)
		RES_SPECIALTYPES,
		RES_NBTYPES,
  SAVEREGS=16} restype;
protected:  
  static Resources* gResources;
};

class Resources : private UsingResources {
  unsigned rescount[RES_NBTYPES];
  bool isSnapshot;
public:
  Resources();
  /** read the current allocation level of a given type */
  unsigned get(restype rt);
  /** reset the current allocation level of a given type */
  void reset(restype rt);

  /** snapshot will receive a copy of the counts recorded here */
  void snap(Resources &snapshot);
  void restore(Resources &snapshot);
  unsigned allocate(restype rt, unsigned nb=1)
;
};

#endif
