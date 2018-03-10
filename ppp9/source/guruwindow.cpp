#include <debug.h>
extern "C" void guruMeditationDump(); // should sit in libnds : gurumeditation.o

Window* theGuru = 0;


static void myGuruHandler() {
  guruMeditationDump();
  if (theGuru) {
    theGuru->restore();
  }
}

GuruWindow::GuruWindow() {
  if (!theGuru) {
    theGuru = this;
    iprintf("beware the guru\n");
    defaultExceptionHandler();
    setExceptionHandler(myGuruHandler) ;
  } else {
    iprintf("dual guru won't do any good\n");
  }
}

void GuruWindow::restore() {
  REG_DISPCNT &= ~DISPLAY_WIN0_ON;
  REG_DISPCNT &= ~DISPLAY_WIN0_ON;
  die("<unknown>",0);
}
