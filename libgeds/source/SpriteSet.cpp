/******************************************************************
 *** SpriteSet management functions for Nintendo DS game engine ***
 ***------------------------------------------------------------***
 *** Copyright Sylvain 'Pype' Martin 2007-2008                  ***
 *** This code is distributed under the terms of the LGPL       ***
 *** license.                                                   ***
 ***------------------------------------------------------------***
 *** last changes:                                              ***
 ***  SpriteSet::getnbtiles required in GameScript::parseline() ***
 ***  SpritePage::changeOAM/setupOAM instead of setOAM required ***
 ***     by GameObject.                                         ***
 *****************************************************************/
#include <nds.h>
#include <nds/arm9/math.h>
#include <nds/arm9/console.h>
#include <nds/arm9/trig_lut.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <fat.h>
#include <errno.h>
#include "SpriteSet.h"
#include "SprFile.h"
#include "debug.h"

/** internal function to unpack the freetiles list from a .SPR file **/
void SpriteRam::unpack(const u16* data, unsigned size) {
  for (unsigned i=0;i<size;i++) {
    u16 v=*data++;
    if ((v&0x3fff) < maxtiles) {
      freetiles[(v>>14)&3].push_back((tileno_t)(v&0x3fff));
    } else {
      warn("ignoring out-of-range freetile %x >= %x\n",v&0x3fff, maxtiles);
    }
    dprint("[%x:%c]",v&0x3fff,"tbBh6?!@"[(v>>14)&7]);
  }
}

/** size in tile for each block type. Backward-compatibility
 *   with existing .SPR files restricts free ordering.
 */
tile_size_t SpriteRam::block_size[SpriteRam::N_SIZES]={1,4,16,8,64};

pageno_t SpriteSet::createPage(u8 w, u8 h) {
  pageno_t npno = (pageno_t)pages.size();
  pages.push_back(SpritePage(w,h));
  dprint("created an empty %ix%i page.\n",w*8,h*8);
  return npno;
}

void SpriteSet::createEmptyPage() {
  pages.push_back(SpritePage(2,2));
  dprint("created an empty 16x16 page.\n");
}

void SpriteSet::createDefaultPage() {
  pages.push_back(SpritePage(2,2));
  for (tileno_t i=TILE0; i<tiles->size(); i+=4)
    pages[0].setTile((blockno_t)(i/4),i);
  dprint("created default page.\n");
}

void SpriteRam::flush() {
  nbtiles=TILE0;
  for (int i=0;i<N_SIZES;i++)
    freetiles[i].clear();
}

void SpriteRam::dumpStats() { 
  int nfree=freetiles[B8x8].size()+
    freetiles[B16x16].size()*4+
    freetiles[B32x32].size()*16+
    freetiles[B64x64].size()*64;
  dprint("%i/%i tiles, %i reusable; ",nbtiles,maxtiles,nfree);
}


/** prefered allocation is to recycle low-id or recently-freed 
 *    locations for the same size. When this isn't possible, 
 *    try_harder_alloc may try:
 **/
tileno_t SpriteRam::try_harder_alloc(enum BlockSize sz, bool maysplit)
{
  const char* psizes[N_SIZES]={"8x8","16x16","32x32", "16x32", "64x64"};
  static const enum BlockSize 
    larger[N_SIZES]={B16x16, B16x32, B64x64, B32x32, B64x64},
    extra[N_SIZES]={B32x32, B32x32, B64x64, B64x64, B64x64};
  
  if (nbtiles+block_size[sz]>=768 && maysplit) {
    /** (only when tileset usage grows high) */
    if (freetiles[larger[sz]].size()>0 || 
	freetiles[extra[sz]].size()>0) {
      /** split a twice-as-large block */
      tileno_t id=alloc(larger[sz],maysplit);
      if (id!=INVALID_TILENO) { /* shouldn't occur */
	freetiles[sz].push_back((tileno_t) (id + block_size[sz]));
	return id;      
      }
    }
  }
  if (nbtiles+block_size[sz]<maxtiles) {
    /** extend the amount of tiles used. */
    tileno_t id = nbtiles;
    nbtiles += block_size[sz];
    return id;
  }
  
  report("oops. no room for %s block in %s pool\n",psizes[sz],pname);
  return INVALID_TILENO;
}

/* if we have the whole file in memory at memdata and that it is
 * datalen byte long, let's just do the job of Load without bothering
 * with the FAT library ^_^
 */
bool SpriteSet::unpack(const u8 *memdata, unsigned datalen) {
  struct SprFileCmap pal_hdr={};
  struct SprFileTile tile_hdr={};
  struct SprFileUnknown hdr;

  pages.clear();  // make sure we start with no pages at all.
  tiles->flush(); // clear free list, etc.
  for (uint i=0;i<3;i++)
    if (extra[i]) extra[i]->flush();

  const u8 *fptr=memdata;
  dprint("cmap->%p, tile->%p\n",palette_target, tiles->raw());
  while (fptr>=memdata && fptr+sizeof(struct SprFileUnknown)<memdata+datalen) {
    //    hdr=*((struct SprFileUnknown *)fptr);
    memcpy(&hdr,fptr,sizeof(struct SprFileUnknown));
    dprint("section '%c%c%c%c': ", MAGIC_PRINT(hdr.magic));
    switch(hdr.magic) {
    case SPR_FILE_CMAP_MAGIC:
      memcpy(&pal_hdr,fptr,sizeof(struct SprFileCmap));
      fptr+=sizeof(pal_hdr);
      pal_hdr.ncols&=0xffff;
      memcpy(palette_target,fptr,pal_hdr.ncols*sizeof(u16));
      fptr+=pal_hdr.ncols*sizeof(u16);
      step("%i colors loaded\n",(int)pal_hdr.ncols);
      break;
    case SPR_FILE_TILE_MAGIC+2:
      dprint("(old version of 'TILE')\n");
    case SPR_FILE_TILE_MAGIC:
      memcpy(&tile_hdr,fptr,sizeof(struct SprFileTile));
      fptr+=sizeof(tile_hdr);

      memcpy(tiles->raw(),fptr,64*tile_hdr.ntiles);
      step("%i tiles loaded\n",(int)tile_hdr.ntiles);
      fptr+=64*tile_hdr.ntiles;
      tiles->setup(tile_hdr.getntiles());
      break;
    case SPR_FILE_TMAP_MAGIC: {
      struct SprFileTmap tmap_hdr;
      memcpy(&tmap_hdr,fptr,sizeof(struct SprFileTmap));
      fptr+=sizeof(tmap_hdr);
      mwidth=tmap_hdr.width;
      mheight=tmap_hdr.height;
      mlayers=(tmap_hdr.skipsize-2*sizeof(u16)) / (mwidth*mheight*sizeof(u16));
      step("loading %ix%ix%i map",mwidth,mheight,mlayers);
      map=(u16*)malloc(mwidth*mheight*sizeof(u16)*mlayers);
      if (map) memcpy(map,fptr,mwidth*mheight*sizeof(u16)*mlayers);
      fptr+=mwidth*mheight*sizeof(u16)*mlayers;
      break;
    }
    case SPR_FILE_FREE_MAGIC: {
      struct SprFileTile fhdr;
      memcpy(&fhdr,fptr,sizeof(struct SprFileTile));
      fptr+=sizeof(struct SprFileTile);
      dprint("%lu free tiles", fhdr.ntiles);
      tiles->unpack((u16*)fptr,fhdr.ntiles);
      break;
    }
    case SPR_FILE_BLCK_MAGIC: {
      struct SprFileBlock bhdr;
      memcpy(&bhdr,fptr,sizeof(struct SprFileBlock));
      fptr+=sizeof(struct SprFileBlock);
      ramno_t ramno=(ramno_t) (bhdr.nblocks>>24);
      bhdr.nblocks&=0xffffff;
      step("%i %ix%i blocks\n",
	   (int)bhdr.nblocks,bhdr.width*8, bhdr.height*8);
      if (ramno >= N_RAMS) {
	warn("unexpected ramno#%i for page #%i\n", ramno, pages.size());
      } else {
	pages.push_back(SpritePage((tile_size_t)bhdr.width,(tile_size_t)bhdr.height,ramno));
      }
      unsigned pnum=pages.size()-1;
      if (pages[pnum].getMax()>=(int)bhdr.nblocks) {
	tileno_t* blocks=(tileno_t*)fptr;

	for(blockno_t i=BLOCK0; i<(blockno_t)bhdr.nblocks; i++)
	  if (blocks[i]!=INVALID_TILENO)  // used for padding.
	    pages[pnum].setTile(i,blocks[i]);
      } else {
	warn("Oops. Only %i slots in page %i\n",pages[pnum].getMax(),pnum);
	//fseek(file,hdr.skipsize-2*sizeof(u32),SEEK_CUR);
      }
      fptr+=sizeof(u16)*bhdr.nblocks;

    }
      break;
    default:
      dprint("unknown. skipping %u bytes\n",(uint)hdr.skipsize);
      fptr+=hdr.skipsize+sizeof(struct SprFileUnknown);
      break;
    }
  }
  if (!pal_hdr.magic || !tile_hdr.magic) {
    report("unexpected end of data");
    return false;
  }

  if (pages.empty()) {
        dprint("we need a default page here.\n");
	createDefaultPage();
  }
  return true;
}

bool SpriteSet::Load(const char *filename, int wantedno, std::vector<DataBlock> *unknown) {
  pages.clear(); // make sure we start with no pages at all.
  tiles->flush();
  palettemask=0;
  for (uint i=0;i<3;i++)
    if (extra[i]) extra[i]->flush();

  // if (!checkFileLibrary()) return false;
  FILE* file=fopen(filename,"rb");
  struct SprFileCmap pal_hdr={};
  struct SprFileTile tile_hdr={};
  struct SprFileUnknown hdr={};
  
  if (!file) {
    report("file missing: %s\n", filename);
    return false;
  }
  fseek(file,0,SEEK_END);
  long at,len=ftell(file);
  fseek(file,0,SEEK_SET);

  step("reading '%s' (%x)...\n",filename,(int)len);
  while ((at=ftell(file))!=len) {
    if (/*feof(file) ||*/ fread(&hdr, sizeof(hdr),1,file)!=1) { 
      fclose(file); 
      report("unexpected EOF (%s,%lu)\n",filename,at);
      return false;
    }
    dprint("@%x '%c%c%c%c' ", (int)at, MAGIC_PRINT(hdr.magic));
    switch(hdr.magic) {
    case SPR_FILE_CMAP_MAGIC:
      pal_hdr.magic=hdr.magic;
      if (fread(&(pal_hdr.ncols),sizeof(u32),1,file)==1) {
	uint paloffset=(pal_hdr.ncols>>24);
	palettemask|=1<<paloffset;
	pal_hdr.ncols&=0xffff;
	if (paloffset!=0) step("set has paloffset#%i\n",paloffset);
	if (paloffset+pal_hdr.ncols<=ncolors) {
	  uint nb=fread(palette_target+paloffset*256,sizeof(u16),pal_hdr.ncols&~1,file);
	  if (pal_hdr.ncols&1) {
	    u16 extracol;
	    fread(&extracol,sizeof(u16),1,file);
	    palette_target[paloffset*256+pal_hdr.ncols-1]=extracol;
	  }
	  step("%i/%i colors loaded as CMAP#%i\n",nb,(int)pal_hdr.ncols,paloffset);
	} else {
	  dprint("ignoring CMAP#%i: only %i colors accepted\n",paloffset,ncolors);
	  fseek(file,hdr.skipsize-sizeof(u32),SEEK_CUR);
	}
      }
      break;

    case SPR_FILE_TILE_MAGIC+'Z'-'T': // known as "draft" by SEDS
    case SPR_FILE_TILE_MAGIC+'Y'-'T': // known as "anim" by SEDS
    case SPR_FILE_TILE_MAGIC+'X'-'T': // known as "sprite" by SEDS
    case SPR_FILE_TILE_MAGIC+2:  // for old versions support
    case SPR_FILE_TILE_MAGIC: {
      int setno;
      SpriteRam *ram=tiles;

      if (hdr.magic <= SPR_FILE_TILE_MAGIC+2) setno=0;
      else setno=1+ (hdr.magic - (SPR_FILE_TILE_MAGIC - 'T' + 'X'));
      step("this is subset #%i\n",setno);
      if (setno!=wantedno) ram=extra[setno-1];
      if (!ram) { 
	step("not interested. skipping tiles.\n"); 
	fseek(file, hdr.skipsize, SEEK_CUR);
	break; 
      }
      
      tile_hdr.magic=hdr.magic;
      if (fread(&(tile_hdr.ntiles),sizeof(u32),1,file)==1) {
	if (tile_hdr.ntiles>ram->max()) {
	  fclose(file);
	  report("oops. %i tiles is far too much. %i max\n",(int)tile_hdr.ntiles,ram->max());
	  return false;
	}
	if (fread(ram->raw(),64,tile_hdr.ntiles,file)==tile_hdr.ntiles)
	  step("%i tiles loaded\n",(int)tile_hdr.ntiles);
      }
      ram->setup(tile_hdr.getntiles());
      break;
    }
    case SPR_FILE_FREE_MAGIC: {
      uint ntiles, ramno;
      if (fread(&ntiles,sizeof(uint),1,file)==1 && ntiles>0) {
	ramno=ntiles>>24;
	ntiles=ntiles&0xffffff;
	if ((ramno?extra[ramno-1]:tiles)) {
	  dprint("%i free tiles on slot#%i@%p\n",ntiles,ramno,
		 (ramno?extra[ramno-1]:tiles));
	  u16* data=(u16*)malloc(ntiles*sizeof(u16));
	  if (data) {
	    if (fread(data,sizeof(u16),ntiles,file)==ntiles)
	      (ramno?extra[ramno-1]:tiles)->unpack(data,ntiles);
	    free(data);
	    dprint(" done.\n");
	  } else report("oops?\n");
	} else { 
	  dprint("oh, btw, no slot #%i defined :P\n",ramno);  
	  fseek(file, ntiles*sizeof(u16), SEEK_CUR);
	}
      } else report("wrong data inside 'FREE'\n");
      break;
    }

    case SPR_FILE_BLCK_MAGIC: {
      struct SprFileBlock bhdr;
      ramno_t ramno;
      bhdr.magic=hdr.magic;
      if (fread(&(bhdr.nblocks),sizeof(u32),2,file)==2 && bhdr.nblocks>0) {
	ramno=(ramno_t)(bhdr.nblocks>>24);
	bhdr.nblocks&=0xffffff;
	step("%i %ix%i blocks\n",
	       (int)bhdr.nblocks,bhdr.width*8, bhdr.height*8);
	pages.push_back(SpritePage((tile_size_t)bhdr.width,(tile_size_t)bhdr.height,ramno));
	unsigned pnum=pages.size()-1;
	if (pages[pnum].getMax()>=(int)bhdr.nblocks) {
	  tileno_t blocks[bhdr.nblocks];
	  if (fread(blocks, sizeof(tileno_t),bhdr.nblocks,file)==bhdr.nblocks) {
	    for(blockno_t i=BLOCK0; i<(blockno_t)bhdr.nblocks; i++)
	      pages[pnum].setTile(i, blocks[i]);
	    // printf("blocks defined\n");
	  }
	} else {
	  // only -1 slots in page 0 show page 0 with 0 items.
	  warn("Oops. Only %i/%i slots in page %i\n",pages[pnum].getMax(), (int)bhdr.nblocks, pnum);
	  fseek(file,hdr.skipsize-2*sizeof(u32),SEEK_CUR);
	}
      }
      break;
    }
    case SPR_FILE_TMAP_MAGIC: {
      struct SprFileTmap tmap_hdr;
      tmap_hdr.magic=hdr.magic;
      tmap_hdr.skipsize=hdr.skipsize;
      if (fread(&(tmap_hdr.width),sizeof(u16),2,file)==2) {
	mwidth=tmap_hdr.width;
	mheight=tmap_hdr.height;
	mlayers=(tmap_hdr.skipsize-2*sizeof(u16)) / (mwidth*mheight*sizeof(u16));
	step("loading %ix%ix%i map. ",mwidth,mheight,mlayers);
	map=(u16*)malloc(mwidth*mheight*sizeof(u16)*mlayers);
	if (map) {
	  int nb=fread(map,sizeof(u16),mwidth*mheight*mlayers,file);
	  dprint("%i/%i cells read.\n",nb,mwidth*mheight*mlayers);
	  if (nb!=mwidth*mheight*mlayers) 
	    report("oops. wrong size");
	  dprint("ok\n");
	  break;
	} else report("oops. Can't allocate %ix%i map", mwidth, mheight);
      }
      report("oops. cannot read tile map");
      break;
    }


    default:
      /*** TODO : store extra sheets here if no "extra tileset" has been
       **    given.
       ****/
      if (unknown) {
	DataBlock db;
	if ((db.ptr=malloc(hdr.skipsize))
	    && fread(db.ptr,1,hdr.skipsize,file)==hdr.skipsize) {
	  db._=hdr;
	  unknown->push_back(db);
	}
      } else {
	dprint("unknown. skipping %u bytes '%c%c%c%c' @%lx\n",(int)hdr.skipsize, MAGIC_PRINT(hdr.magic),at);
	fseek(file, hdr.skipsize, SEEK_CUR);
      }
      break;
    }
  }
  fclose(file);
  dprint("final palette mask: %x\n",palettemask);
  if (pages.empty()) {
    dprint("we need a default page here.\n");
    createDefaultPage();
  }
  opened_filename=filename;
  return true;
}

/** converts tiled data into "flat" NxM target. 
 ** - target should be allocated to proper size. use page.width & page.height to figure.
 ** - caller must compute source of data himself.
 **/
void linearize_sprite_data(u8* target, u16* tile, const SpritePage *page, int inc=-1) {
  unsigned w=page->getWidth();
  unsigned h=page->getHeight();
  unsigned dw=w*8; // how much pixels per line in the external structure.
  if (inc == -1) inc = page->tellInc();

  DC_FlushAll();   // make sure the source made it to the RAM.
  for (unsigned j=0;j<h;j++) {
    for (unsigned i=0;i<8;i++) {          // 8 rows per 'tile row'. that's cst too.
      dmaCopy(tile+i*4, target+i*dw,8); // first tile takes (0,0)-(7,7)
      if (w>1) dmaCopy(tile+32+i*4, target+8+i*dw,8); // second tile (8,0)-(15,7)
      if (w>2) dmaCopy(tile+64+i*4, target+16+i*dw,8);
      if (w>3) dmaCopy(tile+96+i*4, target+24+i*dw,8);
    }
    tile+=w*32; target+=w*64;
  }
  DC_InvalidateRange(target,64*inc);

}

bool SpriteSet::getdata(u8* target, pageno_t pageno, u16 sprid) {
  if (pageno >= (pageno_t)pages.size()) return false;
  const SpritePage *pg = getpage(pageno);
  SpriteRam *ram = pg->getram()?extra[pg->getram()-1]:tiles;
  if (!ram || pg->size() < sprid) return false;
  u16 offset = (*pg)[sprid]; // expressed in tiles (suited for OAM.attribute[2])
  
  linearize_sprite_data(target, ram->raw()+offset*32, pg);
  return true;  
}

bool SpriteSet::setmap(u16* data, world_tile_t w, world_tile_t h, u16 l) {
    if (map) return false;
    dprint("SpriteSet received external %ix%ix%i map.\n",w,h,l);
    map=data; mwidth=w; mheight=h; mlayers=l;
    return true;
  }


bool SpriteSet::setpalette(u16* target, u16 from, u16 to)
{
  
  dmaCopy(palette_target+from,target+from,(to-from)*sizeof(u16));
  return true;
}
