/* -*- C++ -*- */
/******************************************************************
 *** SpriteSet management functions for Nintendo DS game engine ***
 ***------------------------------------------------------------***
 *** Copyright Sylvain 'Pype' Martin 2007-2008                  ***
 *** This code is distributed under the terms of the LGPL       ***
 *** license.                                                   ***
 ***------------------------------------------------------------***
 */
#ifndef __SPRITE_SET_H
#define __SPRITE_SET_H

#define SPR_FILE_ANIM_MAGIC 0x4d494e41 // one animation data
#define SPR_FILE_TINY_MAGIC 0x594e4954 // one animation thumbnail
#include <vector>
#include "debug.h"
#include "UsingSprites.h"
#include "SpriteTypes.h"

/** common header for chunks in a .SPR file */
struct SprFileUnknown {
    u32 magic;
    u32 skipsize;
};

/** identify which storage area we should use for pages */
enum ramno_t { TILES, SPRITES, ANIM, DRAFT, N_RAMS }; 

/** size of something expressed in tiles */
typedef u8 tile_size_t;

class SpriteSheet;

/** a page is a logical set of blocks made of tiles. It stores location
 *   in SpriteRam for each of its blocks. To edit a page, you must first
 *   load it on a SpriteSheet.
 */
class SpritePage : private UsingSprites, private std::vector<tileno_t> {
  //  std::vector<u16> tiles;     // a pointer to each tile in the page.
  tile_size_t height,width; 
  tile_size_t nb;           // number of tiles in the page.
  ramno_t ram;          // the no of the SpriteRam holding tiles.
  int max;

  friend class GobAnim;

 public:
  static unsigned granularity_divisor;
  SpritePage(tile_size_t w, tile_size_t h, ramno_t r=TILES) : 
    height(h), width(w), nb(0), ram(r), max(-1) {
    max=tellMax(w,h);
    if (max>0) reserve(max); // i just need build-time adjustment, this time.
  }
  
  SpritePage(const SpritePage& other) : 
    std::vector<tileno_t>(other),
    height(other.height), width(other.width),nb(other.nb), 
    ram(other.ram), max(other.max) 
  {
    if (max>0) reserve(max);
  }

  ~SpritePage() {
  }
  friend class SpriteSet;
  blockno_t /*std::vector<_Tp, _Alloc>::*/size() const {
    return (blockno_t) std::vector<tileno_t>::size();
  }

  ramno_t getram() const {
    return ram;
  }

 private:
  /** how many blocks can we fit in 16K for w*h blocks? */
  static int tellMax(tile_size_t w, tile_size_t h) {
    static s16 mx[]={-1,8*24,8*12,-1, // 1x1 - 1x2/2x1
		  4*12,-1,3*12,-1,  // 2x2 - 3x2/2x3
		  2*12,3*8,-1,-1,    // 4x2/2x4 - 3x3
		  3*6,-1,-1,-1,     // 4x3/3x4
		  2*6,              // 4x4
    };
    if (w>4 || h>4) return -1;
    return mx[w*h];
  }
 public:
  /** how many tiles should we advance for w*h blocks ? */
  static int tellInc(tile_size_t w, tile_size_t h) {
    static u8 inc[]={
      0,1,2,0,
      4,0,8,0, // 3x2 blocks require a WIDE 16pixel sprite 
      8,16,0,0,
      16,0,0,0,
      16
    };
    if (w>4 || h>4) return 0;
    return inc[w*h];
  }
 
  int tellInc() const {
    return tellInc(width,height);
  }

  int getMax() const { return max; }
  
  // sets block #i as starting at tile #j
  // deferring the creation of the tile array (allocation issue)
  // i guess i need a copy constructor that ensure we only free
  // the malloc'd array once, when all the referring SpritePages
  // are deleted.
  void setTile(blockno_t block_index, tileno_t tile_index) {
    if (block_index==(blockno_t)size()) push_back(tile_index);
    if (block_index<max) (*this)[block_index]=tile_index;
  }
  
  blockno_t addTile(tileno_t tile_index) {
    this->push_back(tile_index);
    return (blockno_t) size();
  }

  tileno_t getTile(blockno_t block_index) const { 
    if (block_index<max) return (*this)[block_index];
    return INVALID_TILENO;
  }

  /** returns the position of the last blocks present on the page */
  unsigned getCount() const { return size(); }
  /** height of blocks on this page, in tiles */
  tile_size_t getHeight() const { return height; }
  /** width of blocks on this page, in tiles */
  tile_size_t getWidth() const { return width; }
  
  bool setOAM(blockno_t sprid, gSpriteEntry *oam) const;
  /** setup frame, x and y, unhide sprite. */
  void changeOAM(gSpriteEntry *oam, u16 x, u16 y, blockno_t sprid) const;
  /** define attributes, hide sprite */
  bool setupOAM(gSpriteEntry *oam, u16 flags=0) const;

};

/** collects information about a storage area for tiles and provides allocation functions
 */
class SpriteRam : iReport {
  NOCOPY(SpriteRam);
public:
  /** WARNING: .spr binary encoding uses these values for telling the
   *   size of the blocks in the FREE session, and it only allows 
   *   4 distinct sizes. If added, B64x64 wouldn't be able to be freed.
   */
  enum BlockSize { B8x8, B16x16, B32x32, B16x32, B64x64, N_SIZES};
protected:
  static tile_size_t block_size[N_SIZES];
  tileno_t maxtiles; /** how many tiles can we have here */
  tileno_t nbtiles;  /** and what is the highest used tile */
  u16* target;   /** where are they stored */
  std::vector<tileno_t> freetiles[N_SIZES];
  const char *pname;
  /* if you want to provide a more sophisticated implementation (e.g.
   * one that splits larger free block when we lack small blocks), 
   * feel free to do so *in those try_harder_... functions*
   */
  virtual tileno_t try_harder_alloc(enum BlockSize sz, bool maysplit=false);

public:
  void pack(u16* data); 
  void unpack(const u16* data, unsigned n);

  void sprck_check();
  void sprck_cleanup();

  SpriteRam(u16 *where, tileno_t smax=INVALID_TILENO, const char* pn="default"):
    maxtiles(smax),nbtiles(TILE0),target(where),pname(pn) {
  }

  /** borrow banks of ram at owner for this spriteram */
  int share(SpriteRam *owner) {
    /** tilebases are expressed in 16K. (256 tiles) **/
    unsigned next = (owner->nbtiles & ~255);
    if (owner->nbtiles & 255) next+=256;
    if (next>maxtiles) return -1;  // failed. Not enough space left.
    target = owner->target+(next*32);
    maxtiles = (tileno_t) (owner->maxtiles - next);
    nbtiles = (tileno_t) 0;
    return next/256; // which bank did we got ?
  }

  void setup(tileno_t n) {
    if (n<=maxtiles) nbtiles=n;
  }
  void compact();
  void dumpStats();

  void mark(u32 *bitmask, size_t nb_uint32s);

  inline void free(int nt, u16 id) {
    switch(nt) {
    case 1: return free(B8x8,id);
    case 4: return free(B16x16,id);
    case 8: return free(B16x32,id);
    case 16: return free(B32x32,id);
    case 64: return free(B64x64,id);
    default:
      report("blocksize %i not supported\n",nt);
    }
  }
    

  inline tileno_t alloc(tile_size_t nt, bool maysplit=false) {
    switch(nt) {
    case 1: return alloc(B8x8,maysplit);
    case 4: return alloc(B16x16,maysplit);
    case 8: return alloc(B16x32,maysplit);
    case 16: return alloc(B32x32,maysplit);
    case 64: return alloc(B64x64,maysplit);
    default:
      report("blocksize %i not supported\n",nt);
      return INVALID_TILENO;
    }
  }
  inline tileno_t alloc(enum BlockSize sz, bool maysplit=false) {
    if (freetiles[sz].size()>0) {
      tileno_t id = freetiles[sz].back();
      freetiles[sz].pop_back();
      return id;
    }
    return try_harder_alloc(sz,maysplit);
  }
  inline void free(enum BlockSize sz, tileno_t id) {
    if (id==0) return;
    static int block_size[N_SIZES]={1,4,16,8,64};
    if ((int)id+block_size[sz]==(int)nbtiles) 
      nbtiles = (tileno_t)(nbtiles - block_size[sz]);
    else freetiles[sz].push_back(id);
  }
  unsigned size() {
    return nbtiles;
  }
  unsigned countFreeTiles() {
    int n=0;
    static int block_size[N_SIZES]={1,4,16,8,64};
    for (int i=0;i<N_SIZES;i++)
      n+=freetiles[i].size() * block_size[i];
    return n;
  }
  unsigned countFreeEntries() {
    int n=0;
    for (int i=0; i<N_SIZES; i++)
      n+=freetiles[i].size();
    return n;
  }
  /** access raw data used managed by this SpriteRam. */
  u16* raw() { return target;}
  unsigned max() { return maxtiles; }
  virtual void flush();
  
  virtual ~SpriteRam() {};
};

//! a unknown data item extracted in memory.
struct DataBlock {
  struct SprFileUnknown _;
  void* ptr; 
};


//! The SpriteSet is what holds your sprites.
/*! you may think of it as an in-ram copy of your .spr file. 
 *  you need to give it a location where tiles are stored
 *  (usually some BG or SPR vram bank) and a location where
 *  the palette is written (usually the screen's palette).
 *  The spriteset is made of SpritePages, and one of them can
 *  be editted at a time through the workplace SpriteSheet.
 */
class SpriteSet : public iReport {
  NOCOPY(SpriteSet);
  u16* map; // for TMAP 
  world_tile_t mwidth, mheight; // for TMAP
  u16 mlayers; // for TMAP
  SpriteRam *tiles;
  SpriteRam *extra[N_RAMS-1];
  u16* palette_target;
  static bool checkFileLibrary(void);
  const char* opened_filename;
  std::vector<SpritePage> pages;
  SpriteSheet *work;
  pageno_t editing;
 public:
  /** indicates how much palette entries can be used in palette_target.
   **   this must be set prior to Load() to be effective.
   **/
  uint ncolors;
  /** indicates which palette slots (256 color each) are filled with valid
   **  data after Load() or before Save() operations.
   **/
  uint palettemask;

  bool getdata(u8* target, pageno_t pageno, u16 sprid);
  u16* rawtiles() { return tiles->raw(); }
  SpriteSet(SpriteRam* til, u16* pal) : 
    map(0),mwidth(0),mheight(0),mlayers(0),
    tiles(til), extra(), palette_target(pal),
    opened_filename(0), pages(), work(0), editing(PAGE0),ncolors(256),
    palettemask(0) {
    extra[0]=0; extra[1]=0; extra[2]=0;
  }
  /* SpriteRams remains under the control of the caller and
   * won't be auto-deleted when spriteset is destroyed.
   */
  SpriteSet* setextra(SpriteRam *xtil, uint which=0) {
    if (which<N_RAMS-1)
      extra[which]=xtil;
    return this;
  }


  

  //! defines the working spritesheet that will be able to edit sprites.
  /*! once it has been defined, you use loadsheet/savesheet to transfer
   *  stuff from the SpriteSet to the working memory (usually VRAM) and
   *  back.
   */
  void withSheet(SpriteSheet *sh) {
    work=sh;
  }
  void loadSheet(const SpritePage* pg, SpriteSheet *sh);
  void loadSheet(pageno_t pageno, SpriteSheet *sh=0);
  void saveSheet();
  //! we have no tiles. let's create a 16x16 page anyway.
  void createEmptyPage(); 
  //! we loaded an old file. Let's create a 16x16 page for it.
  void createDefaultPage(); 

  void changePageRam(pageno_t pno, ramno_t ram);
  pageno_t createPage(u8 w, u8 h); // returns the new pageno.
  pageno_t duplicatePage(pageno_t pageno);
  bool killPage(pageno_t pageno);

  void flush() {
    pages.clear();
    tiles->flush();
    opened_filename=0;
  }

  unsigned getnbtiles() const {
    return tiles->size();
  }

  pageno_t getnpages() const {
    return (pageno_t) pages.size();
  }
  const SpritePage* getpage(pageno_t i) const {
    return &(pages[i]);
  }

  int grab(sImage& img, u16 x, u16 y, int pg=-1);

  //--- file control ---
  bool SaveBack( std::vector<DataBlock> *extra = 0);

  //! unpack data from memory.
  bool unpack(const u8 *memdata, unsigned datalen);

  //! load data from the file. 
  /*! everything that isn't explicitly supported by the SpriteSet
   *  model (such as per-block metadata, animations, maps, etc.)
   *  will be stored as DataBlock in the extra argument (if given)
   *  by default, they are simply ignored.
   */
  bool Load(const char* filename, int wanted = 0, std::vector<DataBlock> *extra = 0);

  /** loads map data from an external .map file, and make as if it was embedded in this 
   *  SpriteSet.
   */
  bool LoadMap(const char* filename);
  //! save data into the file
  /*! Write back information into the file.
   *  in addition to regular info (tileset, palette and block),
   *  we also write back extra information given in the list of 
   *  DataBlocks.
   *  \warning : Load("somename.spr"); Save("somename.spr"); does
   *    not guarantee that you don't lose some information. 
   *    { 
           std::vector extra; 
	   Load("somename.spr",extra); 
	   Save("somename.spr",extra);
   *    } does.
   */ 
  bool Save(const char* filename, std::vector<DataBlock> *extra = 0);
  /** use DMA to clone the current palette into an alternate location. */
  bool setpalette(u16* target, u16 from, u16 to);

  u16 kill(u16 blockid);

  u16* getmap(world_tile_t* w, world_tile_t* h) {
    if (!map) return 0;
    *w=mwidth;
    *h=mheight;
    return map;
  }
  bool setmap(u16* data, world_tile_t w, world_tile_t h, u16 l);
  void dropmap() {
    if (map) free(map);
    map = 0;
    mwidth = 0;
    mheight = 0;
    mlayers = 0;
  }
  int hasmap() { return (map!=0) ? mlayers : 0; }
  void setname(const char* fname) {
   opened_filename=fname;
  }
  const char *filename() {
    return opened_filename;
  }
  virtual ~SpriteSet() {
    if (map) free(map);
  }
};


class SpriteListenerInterface {
  NOCOPY(SpriteListenerInterface);
 protected:
  SpriteListenerInterface *next;
  bool enabled;
 public:
  virtual void event(SpriteSheet *s, blockno_t sprid, bool store)=0;
  void chain(SpriteListenerInterface *sli) {
    next=sli; enabled=true;
  }
  virtual ~SpriteListenerInterface() {}
  SpriteListenerInterface() : next(0),enabled(false) {}
  void enable() { enabled=true;}
  void disable() {enabled=false;}
  bool IsStoreRequest(u16 id) {
    return id & 0x8000;
  }
};


#endif
