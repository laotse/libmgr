/*
 *
 * Universal Output Stream Implementation
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: StreamDump.cpp,v 1.6 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  StreamDump - Interface Class
 *
 * This defines the values:
 *
 */

#include "StreamDump.h"
#include "StreamDump.tag"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define DEBUG_MARK 1
#define DEBUG_DUMP 2

#define DEBUG (DEBUG_MARK | DEBUG_DUMP)
#ifdef DEBUG
# include <stdio.h>
#endif
#include <mgrDebug.h>

using namespace mgr;

m_error_t StreamDump::printf(size_t *out, const char *fmt, ...){
  va_list args;
  char *buffer = NULL;
  m_error_t err = ERR_NO_ERROR;

  if(!out || !fmt) return ERR_PARAM_NULL;
  va_start(args,fmt);
  int written = ::vasprintf(&buffer, fmt, args);
  va_end(args);
  if(written < 0) return ERR_MEM_AVAIL;

  if(written){
    *out = written;
    err = write(buffer,out);
  }

  ::free(buffer);
  return err;
}

/*
 * stdio implemenatation
 *
 */

FileDump::FileDump(FILE *fil){
  f = NULL;

  if(!fil) return;
  int fd = ::fileno(fil);
  if(fd < 0) return;
  fd = ::dup(fd);
  if(fd < 0) return;
  f = ::fdopen(fd, "w");
}
  
FileDump::FileDump(const char *name){
  f = ::fopen(name, "w");
}

m_error_t FileDump::write(const void *data, size_t *s){
  if(!f || !data || !s) return ERR_PARAM_NULL;
  return (1 != ::fwrite(data, *s, 1, f))? ERR_FILE_WRITE : ERR_NO_ERROR;
}

m_error_t FileDump::flush(void) {
  if(!f) return ERR_PARAM_NULL;
  if(::fflush(f)) return ERR_FILE_WRITE;
  return ERR_NO_ERROR;
}

m_error_t FileDump::close(void) {
  if(!f) return ERR_PARAM_NULL;
  int res = ::fclose(f);
  f = NULL;
  return (res)? ERR_FILE_CLOSE : ERR_NO_ERROR;
}

m_error_t FileDump::printf(size_t *out, const char *fmt, ...){
  va_list args;

  if(!f || !out || !fmt) return ERR_PARAM_NULL;
  va_start(args,fmt);
  int written = ::vfprintf(f, fmt, args);
  va_end(args);
  if(written < 0){
    *out = 0;
    return ERR_FILE_WRITE;
  }
  *out = written;
  return ERR_NO_ERROR;
}

const char * FileDump::VersionTag(void) const{
  return _VERSION_;
}


/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]){
  m_error_t res;
  size_t tests, errors;

  tests = errors = 0;


  printf("Test %d: CTOR\n",++tests);
  FileDump stream(stdout);
  if(stream.valid()){
    puts("+++ FileDump::valid() finished OK!");
  } else {
    printf("*** Error: CTORs failed %d\n",errno);    
  }

  printf("Test %d: Print simple String\n",++tests);
  const char *stag = "Hello World\n";
  size_t s = strlen(stag);
  res = stream.write(stag,&s);
  if(ERR_NO_ERROR != res){
    printf("*** Error: write() failed 0x%.4x\n",(int)res);
    perror("*** System error");
  } else {
    printf("+++ FileDump::write() finished OK - %d written!\n",s);
  }

  printf("\n%d tests completed with %d errors.\n",tests,errors);
  printf("Used version: %s\n",stream.VersionTag());

  return 0;  
}


#endif // TEST
