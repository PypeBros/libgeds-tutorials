#ifndef __FILE_READER
#define __FILE_READER

#ifndef __DEBUG_H
#include "debug.h"
#endif

/** provides lines of text from a file on SD card */
class FileReader : public InputReader {
  NOCOPY(FileReader);
  FILE *f;
public:
  FileReader(const char *fname) : f(0) {
    f=fopen(fname,"r");
  }

    /* returns the next, non-empty line chomped in *tgt
     * return value = number of lines read (including empty ones).
    */
  virtual int readline(char *tgt , int sz) {
    if (!f) { 
      return 0;
      /*throw iScriptException("X_x no file.");*/
    }
    int lines=0;
    do {
      if (feof(f)) return 0;
      fgets(tgt,sz-1,f);
      tgt[sz-1]=0; // force termination.
      lines++;
    } while (tgt[0]=='\n'); // "If newline is read, it is stored"
    return lines;
  }
  virtual ~FileReader() {
    iReport::step("-.- file %p closed\n",f);
    fclose(f);
  }
};

/** provides lines of text from a string in memory */
class BufferReader : public InputReader {
  NOCOPY(BufferReader);
  const char *data;
  const char *line;
public:
  BufferReader(const char *src) : data(src), line(src) {}
  virtual int readline(char *l, int sz) {
    int skipped=0;
    while (*line=='\n') { line++; skipped++; }
    const char *eol=strchr(line,'\n');
    if (eol && eol-line<(sz-1)) {
      strncpy(l,line,eol-line);
      l[eol-line]=0;
      line=eol+1;
      return skipped+1;
    } else {
      line=0;
      return 0;
    }
  }
  virtual ~BufferReader() {
    /** data don't belong to the reader, so it is *not*
     ** released when the reader is destroyed ! 
     **/
  }
};

#endif
