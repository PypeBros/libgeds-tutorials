#ifndef __SPRITE_TYPES_H
#define __SPRITE_TYPES_H

#ifndef TYPETEST_PAGENO
typedef u16 pageno_t; //!<  identifies a SpritePage within a SpriteSet
const pageno_t PAGE0 = 0;
const pageno_t UNDEF_PAGENO = -1;
#else
typedef enum { UNDEF_PAGENO = -1, PAGE0 = 0} pageno_t;

inline pageno_t operator++(pageno_t &from, int) {
  pageno_t retval = from;
  if (from != UNDEF_PAGENO)
    from++;
  return retval;
}

inline pageno_t operator--(pageno_t &from, int) {
  pageno_t retval = from;
  if (from != UNDEF_PAGENO && from !=PAGE0)
    from--;
  return retval;
}

inline pageno_t operator+=(pageno_t &from, int inc) {
  pageno_t retval = from;
  if (from != UNDEF_PAGENO)
    from+=inc;
  return retval;
}
#endif

#ifndef TYPETEST_BLOCKNO
typedef int blockno_t; //!< identifies a mxn block within a SpritePage/SpriteSheet
const blockno_t BLOCK0 = 0;
const blockno_t HIGHEST_BLOCKNO = 255;
const blockno_t INVALID_BLOCKNO = -1;
#else
typedef enum {BLOCK0=0, HIGHEST_BLOCKNO=255, INVALID_BLOCKNO=-1} blockno_t;

inline blockno_t operator++(blockno_t &from, int) {
  blockno_t retval = from;
  if (from != INVALID_BLOCKNO && from != HIGHEST_BLOCKNO)
    from++;
  return retval;
}
#endif


#ifndef TYPETEST_TILENO
typedef u16 tileno_t; //!<  identifies a 8x8 tile within video memory
const tileno_t INVALID_TILENO = 65535;
const tileno_t TILE0 = 0;
#else
typedef enum {TILE0=0, HIGHEST_TILENO=65534, INVALID_TILENO} tileno_t;
#endif

#endif
