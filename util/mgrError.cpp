/*
 *
 * Error tracking and reporting system
 *
 * (c) 2005 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: mgrError.cpp,v 1.6 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  mgr::Exception
 *
 * This defines the values:
 *  error codes
 *
 */

#include "mgrError.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "mgrError.tag"

#define DEBUG_ALLOCATE 1
#define DEBUG (DEBUG_ALLOCATE)
#include <mgrDebug.h>

#ifdef DEBUG
# include <stdio.h>
#endif

using namespace std;

static const char *tpl_text = 
    "MGR-Library Exception 0x%.4x in file \"%s\" @ %u";

void mgr::Exception::format(const char * const fmt, va_list args ){
  if(!fmt){
    Explain = NULL;
    return;
  }

  size_t alloc = 0;
  for(const char *c = fmt;*c;++c) 
    alloc += (*c == '%')? 5 : 1;
  ++alloc;
  int written = 0;
  do {
    if(written > (int)alloc - 1) alloc = written + 1;
    ExpBuffer = (char *)realloc(ExpBuffer,alloc);  
    if(ExpBuffer) 
      written = vsnprintf(ExpBuffer, alloc, fmt, args);
  } while(ExpBuffer && (written > (int)alloc - 1));
  if(!ExpBuffer){
    Explain = "could not allocate cause information";
  } else {
    Explain = ExpBuffer;
  }  
}

const char *mgr::Exception::what() const throw() {
  xpdbg(ALLOCATE,"what() called: text=%p\n",text);
  if(text) return text;
  size_t l = (fil)? strlen(fil) : 0;
  if(Explain) l+= strlen(Explain) + 2;
  // %.4x exactly fits the 4 digits for the error code
  l += strlen(tpl_text);
  // add maximum length of size_t typed number (10 bits = 1024 = 4 digits)
  l += ((sizeof(size_t) * 8 + 9) / 10) * 4;
  ++l; // 0 termination
  char **txt = const_cast<char **>(&text);
  char *t2 = (char *)malloc(l);
  xpdbg(ALLOCATE,"what() allocated %u bytes at %p\n",l,t2);
  /* this one fools the compiler, 
   * since text is const, we use t2 for the rest of what()
   * calling what next time will return text as t2
   */
  *txt = t2;
  if(!t2)
    return "MGR-Library exception: Low memory cannot produce error information";
  l = sprintf(t2,tpl_text,(int)cause,(fil)? fil : "",line);
  if(!Explain){
    xpdbg(ALLOCATE,"what() allocate: text=%p,%p used %d bytes\n",
	  text,t2,strlen(t2));
    return text;
  }
  char *t3 = t2 + l;
  *t3++ = ':'; *t3++ = ' ';
  strcpy(t3,Explain);
  xpdbg(ALLOCATE,"what() allocate: text=%p,%p used %d bytes\n",
	text,t2,strlen(t2));
  return t2;
}

const char * mgr::Exception::VersionTag(void){
  return _VERSION_;
}

/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

using namespace mgr;

int main(int argc, char *argv[]){
  // m_error_t res;
  size_t tests, errors;

  tests = errors = 0;

  printf("Test %d: Throw something\n",++tests);
  try {
    mgrThrow(ERR_CANCEL);
  }
  catch(std::exception& e){
    puts(e.what());
  }

  printf("Test %d: Throw something explained\n",++tests);
  try {
    mgrThrowExplain(ERR_CANCEL,"Test case succeeded!");
  }
  catch(std::exception& e){
    puts(e.what());
  }

  printf("\n%d tests completed with %d errors.\n",tests,errors);
  printf("Used version: %s\n",mgr::Exception::VersionTag());

  return 0;  
}


#endif // TEST
