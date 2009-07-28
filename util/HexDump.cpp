/*
 *
 * Universal Output Stream - Filter for HexDump generation
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: HexDump.cpp,v 1.6 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  HexDump - Fileter for StreamDump to create Hex output
 *
 * This defines the values:
 *
 */

#include "HexDump.h"
#include "HexDump.tag"
#include <stdarg.h>

#define DEBUG_MARK 1
#define DEBUG_DUMP 2

#define DEBUG (DEBUG_MARK | DEBUG_DUMP)
#ifdef DEBUG
# include <stdio.h>
#endif
#include <mgrDebug.h>

using namespace mgr;

HexDump::HexDump(StreamDump *o, bool mode){
  out = o;
  textmode = mode;
  prefix = "";
  linepos = 0;
}


m_error_t HexDump::write(const void *data, size_t *s){
  if(textmode){
    return out->write(data, s);
  }

  const char *d = static_cast<const char *>(data);
  size_t read = *s, written = read*3+10;
  m_error_t err = wb.trunc(written);
  if(err != ERR_NO_ERROR) {
    *s = 0;
    return err;
  }

  char *q = wb.writePtr();
  if(!q) return ERR_INT_COMP;
  if(linepos){
    *q++ = ' ';
    written--;
  }
  while(read--){
    if(written < 4){
      size_t offset = wb.indexOf(q);
      err = wb.trunc(wb.byte_size() + read * 3 + 10);
      if( err != ERR_NO_ERROR ){
	*s = 0;
	return err;
      }
      q = wb.writePtr() + offset;
      if(!q) return ERR_INT_COMP;
      written = wb.byte_size() - offset;
    }      
    char c = (*d >> 4) & 0x0f;
    c += (c<10)? '0' : 'a' - 10;
    *q++ = c;
    c = *d++ & 0x0f;
    c += (c<10)? '0' : 'a' - 10;
    *q++ = c;
    *q++ = ' ';
    written -= 3;
    linepos++;
  }
  // overwrite last space with 0x00 terminator
  *(--q) = 0;
  written = wb.indexOf(q);
  //xpdbg(MARK,"Going to write %d bytes: %s\n",written,wb.ptr());
  err = out->write(wb.readPtr(), &written);

  // probably we should check for written and do something to *s

  return err;  
}

m_error_t HexDump::putchar(const void *d) { 
  if(textmode) return out->putchar(d); 
  size_t s = 1;
  return write(d, &s);
}

const char * HexDump::VersionTag(void) const{
  return _VERSION_;
}

m_error_t HexDump::lineFeed(void){
  const char *lf = "\n";
  size_t s_lf = 1;

  linepos = 0;

  return out->write(lf,&s_lf);
}

m_error_t HexDump::textf(size_t *os, const char *fmt, ...){
  va_list args;
  char *buffer = NULL;
  m_error_t err = ERR_NO_ERROR;

  if(!out || !fmt) return ERR_PARAM_NULL;
  va_start(args,fmt);
  int written = ::vasprintf(&buffer, fmt, args);
  va_end(args);
  if(written < 0) return ERR_MEM_AVAIL;

  if(written){
    *os = written;
    err = out->write(buffer,os);
  }

  linepos = 0;

  ::free(buffer);
  return err;

}

/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <errno.h>

int main(int argc, char *argv[]){
  m_error_t res;
  size_t tests, errors;

  tests = errors = 0;


  printf("Test %d: CTORs\n",++tests);
  FileDump stream(stdout);
  HexDump filter(&stream);
  if(filter.valid()){
    puts("+++ HexDump::valid() finished OK!");
  } else {
    printf("*** Error: CTORs failed %d\n",errno);    
  }

  printf("Test %d: Print simple Hex-String\n",++tests);
  // TAG 0e, LEN 2 byte, Content: a5 ff
  const char *stag = "\x1b\x0e\x02\xa5\xff";
  size_t s = strlen(stag);
  res = filter.write(stag,&s);
  if(ERR_NO_ERROR != res){
    printf("*** Error: write() failed 0x%.4x\n",(int)res);
    perror("*** System error");
  } else {
    filter.lineFeed();
    printf("+++ HexDump::write() finished OK - %d written!\n",s);
  }

  printf("\n%d tests completed with %d errors.\n",tests,errors);
  printf("Used version: %s\n",filter.VersionTag());

  return 0;  
}


#endif // TEST
