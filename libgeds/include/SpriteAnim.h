/* -*- C++ -*- */
#ifndef __SPRITE_ANIM
#define __SPRITE_ANIM
#include <nds/ndstypes.h>
#include <vector>
#include <SpriteTypes.h>
class iAnimUser {
 public:
    /** values that can be applied to sprno[i] to force hw limb flipping */
  enum flipper { 
    NOFLIP = 0x3fff,
    XFLIP =  1<<14,
    YFLIP =  2<<14,
  };
  enum commands {
    C_CONTROL,  // flow control statements
    C_DEFINE,   // one-shot properties 
    C_UPDATE,   // frame-by-frame updates
    C_EDITOR   // anything else (if any).
  };
  enum items {
    I_DONE=0, I_DELAY, I_LOOP, I_POP, I_CHECK, I_CONDLOOP,   // controls
    I_BRANCH_GAME, I_BRANCH_GOBBIT,
    I_NBLIMBS=0, I_PAGENO, I_BOX, I_ORIGIN, // defines
    I_SPRNO=0, I_COORDS, I_MOVE, I_MOVETO, I_PULLMASK, I_AREAMASK, I_PROPS, // updates
    I_SPACE=0, // editor-only statement.
  };
  //32   28   24       16               0
  // [CMND|ITEM|.OBJECT.|vvvvvvvvvvvvvvvv|
  static inline unsigned encode(commands c, items i, u8 object, u16 value) {
    return (((unsigned)c)<<28) | (((unsigned) i)<<24) | (object<<16) | value;
  }
  static inline commands command(unsigned u) {
    return (commands) (u>>28);
  }
  static inline items item(unsigned u) {
    return (items) ((u>>24)&0xf);
  }
  static inline unsigned object(unsigned u) {
    return (u>>16)&0xff;
  }

  static inline u16 s8tou16(int a, int b) {
    unsigned ua = a&0xff, ub=b&0xff;
    return ua<<8|ub;
  }

  static inline void u16tos8(unsigned r, s8 *a, s8 *b) {
    u8 ua = (r>>8)&0xff;
    u8 ub = r&0xff;
    *a = (s8) ((r&0x8000)?ua-256:ua);
    *b = (s8) ((r&0x80)?ub-256:ub);
  }

  static inline void u16tos8(unsigned r, s16 *a, s16 *b) {
    u8 ua = (r>>8)&0xff;
    u8 ub = r&0xff;
    *a = (s16) ((r&0x8000)?ua-256:ua);
    *b = (s16) ((r&0x80)?ub-256:ub);
  }
  static inline void u16tos8(unsigned r, s32 *a, s32 *b) {
    u8 ua = (r>>8)&0xff;
    u8 ub = r&0xff;
    *a = (s32) ((r&0x8000)?ua-256:ua);
    *b = (s32) ((r&0x80)?ub-256:ub);
  }
#ifndef DEVKITPRO32
#ifndef uint
  static inline void u16tos8(unsigned r, int *a, int *b) {
    u16tos8(r, (s32*)a, (s32*)b);
  }
#endif
#endif
  /** fills list with ordering implied by the pullmask msk
   */
  static inline void mask2list(uint msk, uint n, char *list) {
    uint i=0;
    for (uint m=msk,j=0; m ;m=m>>1,j++)
      if (m&1)
	list[i++]=j;
    for (uint m=msk,j=0; j<n ;m=m>>1,j++)
      if (!(m&1))
	list[i++]=j;
  }

  /** is c's index different in from[] and to[] ?
   **  (negative means c has a lower index in to[])
   */
  static inline int deltapos(char c, uint n, char* from, char* to) {
    for (uint i=0;i<n;i++) {
      if (from[i]==to[i] && from[i]==c) return 0;
      if (from[i]==c) return 1;
      if (to[i]==c) return -1;
    }
    return 0; // missing in both
  }

  virtual ~iAnimUser() {}
};

#endif
