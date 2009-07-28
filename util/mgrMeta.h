/*
 *
 * Template Metaprogramming handy
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: mgrMeta.h,v 1.4 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *
 * This defines the values:
 *
 */

#ifndef _UTIL_MGR_META_H_
# define _UTIL_MGR_META_H_

#include <stdlib.h>
#include <unistd.h>

namespace meta {
  // basic if then else control structure
  template< bool Condition, typename THEN, typename ELSE > struct IF {
    template< bool C, typename T, typename E > 
      struct selector {typedef T SELECT_CLASS;}; 
    
    template< typename T, typename E > 
      struct selector< false, T, E >
      {typedef E SELECT_CLASS;};     
    
    typedef typename selector< Condition, THEN, ELSE >::SELECT_CLASS RESULT;
  };

  // count number of bits for positive variable
  // negatives are sizeof(B)*8 ...
  template< size_t B > struct NumBits {
    enum{ VALUE = 1 + NumBits< (B >> 1) >::VALUE };
  };
  template<> struct NumBits<0>{
    enum{ VALUE = 0 };
  };

  // count number of bits set in a byte
  template< unsigned char byte > struct BitsInByte {
    enum {
      B0 = (byte & 0x01) ? 1:0,
      B1 = (byte & 0x02) ? 1:0,
      B2 = (byte & 0x04) ? 1:0,
      B3 = (byte & 0x08) ? 1:0,
      B4 = (byte & 0x10) ? 1:0,
      B5 = (byte & 0x20) ? 1:0,
      B6 = (byte & 0x40) ? 1:0,
      B7 = (byte & 0x80) ? 1:0
    };
    enum{RESULT = B0+B1+B2+B3+B4+B5+B6+B7};
  };

  // integer numbers
  template< ssize_t I > struct Number {
    enum{ VALUE = I };
  };
};

#endif // _UTIL_MGR_META_H_
