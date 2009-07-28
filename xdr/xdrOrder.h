/*
 *
 * RFC 1832 - Sun XDR
 * Data Types and endianess module, 
 * i.e. from little endian to machine native
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: xdrOrder.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *
 * xdrIO - set of conversion functions
 *
 * This defines the values:
 *
 */

#ifndef _XDR_XDRORDER_H_
# define _XDR_XDRORDER_H_

#include <unistd.h>
#include <machine/xdr.h>
#include <mgrError.h>

namespace mgr {

namespace XDR {
  template< typename T> inline T xdrRead(const void *d) {
#ifdef XDR_BIG_ENDIAN
    return *((const T *)d);
#elif defined(XDR_LITTLE_ENDIAN)
    //Conv<T> u;
    union { T i; char c[sizeof(T)]; } u;
    char *w = u.c + sizeof(T);
    const char *r = (const char *)d;
    for(size_t i=0;i<sizeof(T);i++){
      *(--w) = *r++;
    }
    return u.i;
#else
# error "XDR endianess undefined or unknwon"
#endif
  }

  template< typename T> inline void xdrWrite(void *d, const T& v) {
#ifdef XDR_BIG_ENDIAN
    *((const T *)d) = v;
#elif defined(XDR_LITTLE_ENDIAN)
    //const Conv<T> u = {i: v};
    const union { T i; char c[sizeof(T)]; } u = { v };
    const char *w = u.c + sizeof(T);
    char *r = (char *)d;
    for(size_t i=0;i<sizeof(T);i++){
      *r++ = *(--w);
    }
#else
# error "XDR endianess undefined or unknwon"
#endif
  }  

  // types XDR::(U)Type
#define XDR_TYPE(P, T, N) \
  typedef P XDR_ ## T N

#define XDR_TYPE_PAIR(T,N) XDR_TYPE(,T,N); XDR_TYPE(unsigned, T, U ## N)

  XDR_TYPE_PAIR(INT,Int);
  XDR_TYPE_PAIR(CHAR,Char);
  XDR_TYPE_PAIR(SHORT,Short);
  XDR_TYPE_PAIR(LONG,Long);
  XDR_TYPE(,FLOAT,Float);
  XDR_TYPE(,DOUBLE,Double);

}

class xdrIO {
public:
  /*
   * Sorry, this is not supported in C++
   *
  template< typename T > typedef union {
    T i;
    char c[sizeof(T)];
  } Conv;
  */


  // The real interface starts here

  const char * VersionTag(void) const;

  // OK, now follows the implementation for each supported data type
  // 1) readers
#define XDR_READER(P,T,N) \
  inline P XDR_ ## T read ## N (const void *d) const { \
    return XDR::xdrRead< P XDR_ ## T >(d); \
  }
#define XDR_READ_PAIR(T,N) XDR_READER(,T,N) XDR_READER(unsigned, T, U ## N)

  inline XDR_CHAR readChar(const void *d){
    return *((const XDR_CHAR *)d);
  }
  inline unsigned XDR_CHAR readUChar(const void *d){
    return *((const unsigned XDR_CHAR *)d);
  }

  XDR_READ_PAIR(INT,Int)
  XDR_READ_PAIR(LONG,Long)
  XDR_READ_PAIR(SHORT,Short)
  XDR_READER(,FLOAT,Float)
  XDR_READER(,DOUBLE,Double)


  // 2) writers
#define XDR_WRITER(P,T,N) \
  inline void write ## N (void *d, const P XDR_ ## T & i) const{ \
    XDR::xdrWrite< P XDR_ ## T >(d,i); \
  }
#define XDR_WRITE_PAIR(T,N) XDR_WRITER(,T,N) XDR_WRITER( unsigned, T, U ## N)

  inline void writeChar(void *d, const char& i) const {
    *((char *)d) = i;
  }
  inline void writeUChar(void *d, const unsigned char& i) const {
    *((unsigned char *)d) = i;
  }

  XDR_WRITE_PAIR(INT,Int)
  XDR_WRITE_PAIR(SHORT,Short)
  XDR_WRITE_PAIR(LONG,Long)
  XDR_WRITER(,DOUBLE,Double)
  XDR_WRITER(,FLOAT,Float)  

  // check, if the settings in <machine/xdr.h> were chosen correctly
  m_error_t check(void) const;

#undef XDR_READER
#undef XDR_READ_PAIR
#undef XDR_WRITER
#undef XDR_WRITE_PAIR
#undef XDR_TYPE
#undef XDR_TYPE_PAIR

};
} // namespace mgr

#endif // _XDR_XDRORDER_H_
