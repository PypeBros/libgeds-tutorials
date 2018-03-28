#ifndef GAME_CONSTANTS_H
#define GAME_CONSTANTS_H
#include <interfaces.h>

/** ranges for whoever is using scripted levels */
class UsingParsing {
public:
  static const gobno_t MAXGOBS  = (gobno_t) 99; // only limits the number of *static* GOBs.
  static const unsigned MAXANIMS =64;
  static const unsigned MAXSTATES=64;
  static const unsigned MAXCOUNTERS=16;
  static const unsigned MEDS_MAXANIMS=512;
  static bool safecheck();
protected:
  /** the path where we look up for the next script (and resource files)
   */
  static const char* default_dir;
  static void set_default_dir(const char *dd) { default_dir=dd; }
  static char* content(char* buf);
};
#endif
