#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#ifndef __DATAREADER_H__
#define __DATAREADER_H__
class DataReader {
 public:
  bool gotwrong;
  DataReader() : gotwrong(false) {}
  virtual unsigned tellsize()=0;
  virtual int read(void *where, unsigned size)=0;
  virtual int skip(int nb)=0;
  virtual u32 U32()=0;
  virtual u16 U16()=0;
  virtual u8 U8()=0;
  virtual ~DataReader() {};
};

class MemDataReader : public DataReader {
  char *data;
  char *curr;
  unsigned size;
 public:
  MemDataReader(void *_data, unsigned _size):
    data((char*)_data), curr(data), size(_size)
    {
  }
  virtual unsigned tellsize() {
    return size;
  }
  virtual int read(void *where, unsigned sz) {
    memcpy(where,curr,sz);
    curr+=sz;
    if (curr>data+size) gotwrong=true;
    return sz;
  }
  virtual int skip(int nb) {
    curr+=nb;
    if (nb<0 || curr>data+size) gotwrong=true;
    return nb;
  }

  virtual u32 U32() {
    u32 t; 
    memcpy(&t,curr,4);
    curr+=4;
    if (curr>data+size) gotwrong=true;
    return t;
  }
  virtual u16 U16() {
    u16 t; 
    memcpy(&t,curr,2);
    curr+=2;
    if (curr>data+size) gotwrong=true;
    return t;
  }
  virtual u8 U8() {
    u8 t; 
    memcpy(&t,curr,1);
    curr++;
    if (curr>data+size) gotwrong=true;
    return t;
  }
  virtual ~MemDataReader() {};
};

class FileDataReader : public DataReader {
  const char *filename;
  FILE *file;
 public:
  FileDataReader(const char* name) : 
    filename(name), file(0) 
    {
      file=fopen(name,"rb");
      gotwrong=(file==0);
      if (gotwrong) iprintf("couldn't open '%s'\n",name);
    }
    
    virtual ~FileDataReader() {
    if (file) fclose(file);
  }
  
  virtual unsigned tellsize() {
    struct stat fstats;
    if (gotwrong) return 0;
    stat(filename, &fstats);
    u32 filesize = fstats.st_size;
    return filesize;
  }
  
  virtual int read(void* where, unsigned size) {
    int n= fread(where,1,size,file);
    if (n<0) gotwrong=true;
    return n;
  }
  virtual int skip(int nb) {
    if (gotwrong) return 0;
    int n=fseek(file,nb,SEEK_CUR);
    if (n<0) gotwrong=true;
    return n;
  }
  virtual u32 U32() {
    u32 t;
    if (fread(&t,4,1,file)!=1) gotwrong=true;
    return t;
  }
  virtual u16 U16() {
    u16 t;
    if (fread(&t,2,1,file)!=1) gotwrong=true;
    return t;
  }
  virtual u8 U8() {
    u8 t;
    if (fread(&t,1,1,file)!=1) gotwrong=true;
    return t;
  }
};
#endif
