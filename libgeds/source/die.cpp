#include "nds.h"
#include <nds/arm9/console.h> //basic print funcionality
#include <stdio.h>

#include "GuiEngine.h"
#include <malloc.h>
#include <nds/arm9/exceptions.h>

extern "C" u32 __malloc_av_[];
extern "C" u32 __bss_start[];
#ifdef DEVKIPRO_HAS_BSS_END
extern "C" u32 __bss_end[];
#endif

void die(const char* file, int line) {
  unsigned addr=exceptionRegisters[13];
  unsigned step=0x10;
  unsigned displayed = addr;
  BG_PALETTE_SUB[0]=RGB15(0,0,31);
  BG_PALETTE[0]=RGB15(0,0,31);
  if (addr==0) addr = (unsigned)__malloc_av_;

         /*01234567890123456789012345678901*/
  if (line) {
    iprintf("**** DOH! assertion failed ****\n");
    iprintf("%s : %i\n",file,line);
  } else {
    iprintf("**** DOH! %s ****\n",file);
  }
  
  unsigned keymask;

  if ((REG_IME&1)==IME_DISABLE) {
    BG_PALETTE_SUB[0]=RGB15(31,0,0);
    iprintf(CON_GOTO(1,1) " IRQ - %x %x", (int)REG_IE, (int)REG_IF);
    keymask=0x3ff; // only GBA keys are available.
  } else {
    if (exceptionRegisters[13]==0) {
      keymask=0x3ff;
    } else {
      keymask=0x7ff;
    }
  }

  int cycles=0;
  while(1) {
    BG_PALETTE_SUB[0]=(BG_PALETTE_SUB[0] & 0xfc1f) | ((cycles & 0xf)<<5);
    while(keysHeld()&keymask) {
      scanKeys();
    }
    REG_BG0HOFS_SUB=0 ; REG_BG0VOFS_SUB=0;
    cycles+=4;
    while(!(keysHeld()&keymask)) scanKeys();

    if ((keysDown()&KEY_LEFT) && step!=0x1000000) step=step<<4;
    if ((keysDown()&KEY_RIGHT) && step!=0x10) step=step>>4;
    if (keysDown()&KEY_DOWN) addr+=step;
    if (keysDown()&KEY_UP) addr-=step;

    iprintf("\x1b[13;2H----%08x+-%08x----",addr,step);
    if (addr!=displayed) {
      u32* d32 = (u32*) addr;
      char show[]=".... ....";
      if ((addr>= 0x02000000 && addr<=0x023ffff0) ||
	  (addr>= 0x0b000000 && addr<=0x0b00FFF0)) {
	for (int i=0; i<16 ; i+=2) {
	  memcpy(show,d32+i,4);
	  memcpy(show+5,d32+i+1,4);
	  for (int j=0;j<9;j++) if (show[j]<16) show[j]='.';
	  iprintf("\x1b[%d;1H%02x %08x %08x %s\n",
		  14+ (i/2), (addr+i*4)&0xff, (int) d32[i], (int) d32[i+1], show); 
	}
      } else {
	for (int i=0; i<80 ; i+=8) {
	  iprintf("\x1b[%d;1H%02x xxxxxxxx xxxxxxxx .... ....\n", i, (addr+i*4)&0xff);
	}
      }
      displayed=addr;
    }

    if (keysHeld()&KEY_B) {
      // this one is for debugging on upper screen
      consoleInit(0,0,BgType_Text4bpp,BgSize_T_256x256,GECONSOLE,0,true,true);
      //      consoleInitDefault(WIDGETS_CONSOLE, (u16*)CHAR_BASE_BLOCK(0), 16);
      videoSetMode(  MODE_0_2D | 
		 DISPLAY_BG0_ACTIVE    //turn on background 0 for text console
		 );
      REG_BG0CNT = BG_MAP_BASE(GECONSOLE)|BG_TILE_BASE(0)|BG_PRIORITY(0); 

      iprintf("reason:");
      iReport::diagnose();
    }
    if (keysHeld()&KEY_Y&keymask) {
      consoleInit(0,0,BgType_Text4bpp,BgSize_T_256x256,GECONSOLE,0,true,true);

      //consoleInitDefault(WIDGETS_CONSOLE, (u16*)CHAR_BASE_BLOCK(0), 16);
      videoSetMode(  MODE_0_2D | 
		 DISPLAY_BG0_ACTIVE    //turn on background 0 for text console
		 );
      iprintf("malloc_av = %p\n",__malloc_av_);
      iprintf("bss_start = %p\n",__bss_start);
#ifdef DEVKITPRO_HAS_BSS_END
      iprintf("bss_end   = %p\n",__bss_end);
#endif
    }
    if ((keysHeld()&KEY_SELECT)) {
      /** note: WiFi transfers are not reliable in this case. **/
      iReport::action("return to launcher."); // dkp-32 allows this.
      exit(0);
    } else if (keysHeld()&KEY_START) {
    }
  }
}
