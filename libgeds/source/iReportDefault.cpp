#include <cstdarg>
#include "debug.h"
int iReport::reppos=0;
char iReport::repbuf[4096];
#ifndef NO_DEFAULT_ACTION
void iReport::action(const char *fmt, ...)
{
  va_list ap;
  va_start( ap,fmt );
  reppos = vsniprintf( repbuf, 4096, fmt, ap);
  va_end(ap);
  reppos++;
}
#endif
#ifndef NO_DEFAULT_STEP
void iReport::step(const char *fmt, ...)
{
  va_list ap;
  va_start( ap,fmt );
  if (reppos<4096) 
    reppos += vsniprintf( repbuf+reppos, 4096-reppos, fmt, ap);
  else iprintf("buffer too small [%s]\n@%s",fmt,repbuf);
  va_end(ap);
}
#endif
#ifndef NO_DEFAULT_REPORT
bool iReport::report(const char *fmt, ...)
{
  va_list ap;
  va_start( ap,fmt );
  if (reppos<4096)
    reppos += vsniprintf( repbuf+reppos, 4096-reppos, fmt, ap);
  va_end(ap);
  diagnose();
  return false;
}
#endif
#ifndef NO_DEFAULT_WARN
void iReport::warn(const char *fmt, ...)
{
  va_list ap;
  strncpy(repbuf + reppos, "/!\\ ", 4096 - reppos);
  reppos += 4;
  if (reppos > 4096) reppos = 4096;
  va_start( ap,fmt );
  if (reppos<4096) 
    reppos += vsniprintf( repbuf+reppos, 4096-reppos, fmt, ap);
  else iprintf("buffer too small [%s]\n@%s",fmt,repbuf);
  va_end(ap);
}
#endif
