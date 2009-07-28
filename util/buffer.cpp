/*
 *
 * Extensible buffer
 *
 * (c) 2005 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: buffer.cpp,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  mgrBuffer  - an extensible auto-allocatin buffer for bytes
 *
 * This defines the values:
 *
 */

#include <string.h>
#include <stdlib.h>

#include "buffer.h"
#include "buffer.tag"

//#define DEBUG
#ifdef DEBUG
# include <stdio.h>
#endif


#define BUFFER_DEFAULT_CHUNK 1024

inline size_t round_buffer(size_t req, size_t chunk){
  size_t res;
  
  res = req % chunk;
  if(res){
    res = chunk - res;
    req += res;
  }

  return req;
}

mgrBuffer::mgrBuffer(void){
  buffer = NULL;
  used = 0;
  allocated = 0;
  chunk = BUFFER_DEFAULT_CHUNK;
  error = ERR_NO_ERROR;
}

mgrBuffer::mgrBuffer(size_t s){
  size_t n;

  mgrBuffer();
  n = round_buffer(s, BUFFER_DEFAULT_CHUNK );
  buffer = (char *) malloc(n);
  if(!buffer) error = ERR_MEM_AVAIL;
  else allocated = n;
  used = s;
}

char *mgrBuffer::get(size_t s){
  size_t n;

  if(s>allocated){
    n = round_buffer(s,chunk);
#ifdef DEBUG
    printf("Reallocate buffer(%d) from %d(%d) to %d(%d)\n",
	   chunk, allocated, used, n, s);
#endif
    buffer = (char *)realloc(buffer, n);
    if(!buffer){
      allocated = 0;
      error = ERR_MEM_AVAIL;
      return NULL;
    } else {
      allocated = n;    
      // maybe zeroize here
    }
  }
  used = s;

  return buffer;
}

mgrBuffer::~mgrBuffer(void){
  free();
}

char *mgrBuffer::get(void){
  if(buffer && used) return buffer;
  return (char *)NULL;
}

char *mgrBuffer::get(size_t s, size_t off){
  if(!get(s+off)) return (char *)NULL;
  return &(buffer[off]);
}

m_error_t mgrBuffer::free(void){
  if(buffer){
    ::free(buffer);
    buffer = NULL;
  }
  allocated = 0;
  used = 0;

  return ERR_NO_ERROR;
}

m_error_t mgrBuffer::trunc(void){
  size_t n;

  if(allocated - used < chunk) return ERR_NO_ERROR;
  n = round_buffer(used,chunk);
  buffer = (char *)realloc(buffer, n);
  if(!buffer){
    allocated = 0;
    error = ERR_MEM_AVAIL;
    return error;
  } else allocated = n;    
  
  return ERR_NO_ERROR;
}

m_error_t mgrBuffer::trunc(size_t s){
  if(s > used) return ERR_PARAM_RANG;
  used = s;
  
  return trunc();
}

m_error_t mgrBuffer::replace(const char *val, size_t s){
  size_t n;

  if(s > allocated){
    if(buffer) ::free(buffer);
    allocated = 0;
    n = round_buffer(s,chunk);
    buffer = (char *)malloc(n);
    if(!buffer){
      error = ERR_MEM_AVAIL;
      used = 0;
      return error;
    }
    allocated = n;
  }
  memcpy(buffer,val,s);
  used = s;

  return ERR_NO_ERROR;
}

m_error_t mgrBuffer::append(const char *val, size_t s){
  size_t off;

  off = used;
  if(!get(s+used)) return error;
  memcpy(&(buffer[off]),val,s);

  return ERR_NO_ERROR;
}

m_error_t mgrBuffer::prepend(const char *val, size_t s){
  size_t off;

  off = used;
  if(!get(s+used)) return error;
  memmove(&(buffer[s]),buffer,off);
  memcpy(buffer,val,s);

  return ERR_NO_ERROR;
}
  
m_error_t mgrBuffer::setChunk(size_t c){
  chunk = c;

  return ERR_NO_ERROR;
}

const char * mgrBuffer::VersionTag(void){
  return _VERSION_;
}


/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <stdio.h>

int main(int argc, char *argv[]){
  char *s;
  m_error_t res;
  size_t tests, errors;

  tests = errors = 0;

  printf("Test %d: Write string to buffer\n",++tests);
  mgrBuffer temp;
  res = temp.replace("This is a minimal buffer test");
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: replace() failed 0x%.4x\n",(int)res);
  } else {
    printf("??? %s\n",temp.get());
    puts("+++ replace() finished OK!");
  }

  printf("Test %d: Append string by trunc() append()\n",++tests);
  do{
    // truncate trailing 0
    res = temp.trunc(temp.size()-1);
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: trunc() failed 0x%.4x\n",(int)res);
      break;
    }
    s=" Something appended!";
    res = temp.append(s,strlen(s)+1);
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: append() failed 0x%.4x\n",(int)res);
      break;
    }
    s = temp.get();
    if(strlen(s)+1 != temp.size()){
      printf("*** Error: Size is %d was expected as %d\n",temp.size(),strlen(s)+1);
      errors++;
      break;
    }
    printf("??? %s\n",temp.get());
    puts("+++ append() finished OK!");    
  }while(0);

  printf("Test %d: Prepend string by trunc() append()\n",++tests);
  s="This is prepended... ";
  do{
    res = temp.prepend(s,strlen(s));
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: prepend() failed 0x%.4x\n",(int)res);
      break;
    }
    s=temp.get();
    if(strlen(s)+1 != temp.size()){
      printf("*** Error: Size is %d was expected as %d\n",temp.size(),strlen(s)+1);
      errors++;
      break;
    }
    printf("??? %s\n",temp.get());
    puts("+++ prepend() finished OK!");    
  }while(0);

  temp.free();
  temp.setChunk(10);
  puts("Reduced chunk size to 10 byte and repeat!");

  printf("Test %d: Write string to buffer\n",++tests);
  res = temp.replace("This is a minimal buffer test");
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: replace() failed 0x%.4x\n",(int)res);
  } else {
    printf("??? %s\n",temp.get());
    puts("+++ replace() finished OK!");
  }

  printf("Test %d: Append string by trunc() append()\n",++tests);
  do{
    // truncate trailing 0
    res = temp.trunc(temp.size()-1);
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: trunc() failed 0x%.4x\n",(int)res);
      break;
    }
    s=" Something appended!";
    res = temp.append(s,strlen(s)+1);
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: append() failed 0x%.4x\n",(int)res);
      break;
    }
    s = temp.get();
    if(strlen(s)+1 != temp.size()){
      printf("*** Error: Size is %d was expected as %d\n",temp.size(),strlen(s)+1);
      errors++;
      break;
    }
    printf("??? %s\n",temp.get());
    puts("+++ append() finished OK!");    
  }while(0);

  printf("Test %d: Prepend string by trunc() append()\n",++tests);
  s="This is prepended... ";
  do{
    res = temp.prepend(s,strlen(s));
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: prepend() failed 0x%.4x\n",(int)res);
      break;
    }
    s=temp.get();
    if(strlen(s)+1 != temp.size()){
      printf("*** Error: Size is %d was expected as %d\n",temp.size(),strlen(s)+1);
      errors++;
      break;
    }
    printf("??? %s\n",temp.get());
    puts("+++ prepend() finished OK!");    
  }while(0);

  printf("\n%d tests completed with %d errors.\n",tests,errors);
  printf("Used version: %s\n",temp.VersionTag());

  return 0;  
}

#endif //TEST
