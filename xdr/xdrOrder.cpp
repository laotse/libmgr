/*
 *
 * RFC 1832 - Sun XDR
 * Data Types and endianess module, 
 * i.e. from little endian to machine native
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: xdrOrder.cpp,v 1.6 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *
 * xdrIO - set of conversion functions
 *
 * This defines the values:
 *
 */

#include <xdrOrder.h>
#include <string.h>
#include "xdrOrder.tag"

using namespace mgr;

const char * xdrIO::VersionTag(void) const {
  return _VERSION_;
}

m_error_t xdrIO::check(void) const {
  if(sizeof(XDR::Char) != 1) return ERR_INT_COMP;
  if(sizeof(XDR::Short) != 2) return ERR_INT_COMP;
  if(sizeof(XDR::Int) != 4) return ERR_INT_COMP;
  if(sizeof(XDR::Long) != 8) return ERR_INT_COMP;
  if(sizeof(XDR::Float) != 4) return ERR_INT_COMP;
  if(sizeof(XDR::Double) != 8) return ERR_INT_COMP;

  char b[sizeof(XDR::Long)];
  const char s[sizeof(XDR::Long)] = { 0xfe, 0xdc, 0xba, 0x98,
				      0x76, 0x54, 0x32, 0x10 };
  writeULong(b,0xfedcba9876543210ULL);
  if(memcmp(b,s,sizeof(XDR::Long))) return ERR_INT_COMP;

  return ERR_NO_ERROR;
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
  m_error_t res;
  size_t tests, errors;

  tests = errors = 0;

  xdrIO xdr;

  printf("Test %d: Read a long order\n",++tests);
  const char *s = "012345678";
  XDR::ULong l = xdr.readULong(s);
  if( l != 0x3031323334353637ULL ){
    printf("??? l: %llx\n",l);
    printf("*** Error: readULong() failed\n");
  } else {
    puts("+++ BerTag::readULong() finished OK!");
  }
  
  printf("Test %d: Check\n",++tests);
  res = xdr.check();
  if(res != ERR_NO_ERROR){
    errors++;
    printf("*** Error: check() failed 0x%.4x\n",(int)res);
  } else {
    puts("+++ BerTag::check() finished OK!");
  }

  printf("\n%d tests completed with %d errors.\n",tests,errors);
  printf("Used version: %s\n",xdr.VersionTag());

  return 0;  
}

#endif // TEST
