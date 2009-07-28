/*
 *
 * BER, DER, TLV, ASN.1, ... Trees
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: BerTree-meta.h,v 1.4 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  BerTag  - node for a TAG
 *  BerTree - the XTree of berTag
 *
 * This defines the values:
 *
 */

// only from within BerTree.h
#ifdef _TLV_BERTREE_H_
# ifndef _TLV_BERTREE_META_H_
#  define _TLV_BERTREE_META_H_


public:
  // shifts for ID values in 1st Tag byte
  static const size_t type_shift  = 5;
  static const size_t class_shift = 6;
  // Type IDs
  static const unsigned char type_primitive    
  = (unsigned char)BER_PRIMITIVE   << type_shift;
  static const unsigned char type_constructed  
  = (unsigned char)BER_CONSTRUCTED << type_shift;
  static const unsigned char type_mask         
  = (unsigned char)BER_CONSTRUCTED << type_shift;
  // Class IDs
  static const unsigned char class_universal   
  = (unsigned char)BER_UNIVERSAL   << class_shift;
  static const unsigned char class_application 
  = (unsigned char)BER_APPLICATION << class_shift;
  static const unsigned char class_context     
  = (unsigned char)BER_CONTEXT     << class_shift;  
  static const unsigned char class_private     
  = (unsigned char)BER_PRIVATE     << class_shift;  
  static const unsigned char class_mask        
  = (unsigned char)BER_PRIVATE     << class_shift;  


public:
  // Meta Coding Interface

  template< size_t T > struct TagLength {
    enum{ VALUE = meta::IF< (T < 0x1f), 
	    meta::Number<1>, 
	    meta::Number< 1 + (meta::NumBits<T>::VALUE + 6) / 7 > >::RESULT::VALUE };
  };

  template< size_t T, size_t B > struct _STagByte {
    enum{ VALUE = meta::IF< (B == 0), 
	    meta::Number< (T & 0x1f) >, 
	    meta::Number< 0 > >::RESULT::VALUE };
  };

  // Meta Coding helpers
  template<unsigned char N, BerTagType T, BerTagClass C> class _TagVal{
  public:
    enum {VALUE = ( N | T << type_shift | C << class_shift ) };
  };

  template< size_t T, size_t B, int X > struct _LTagByte {
    enum { PLAIN = meta::Number< ((T >> ((TagLength< T >::VALUE - (B + 1)) * 7)) & 0x7f) >::VALUE };
    enum { VALUE = meta::IF< (X == 0), 
	   meta::Number< PLAIN >, 
	   meta::Number< (PLAIN  | 0x80) > >::RESULT::VALUE };
  };
  template< size_t T, int X> struct _LTagByte< T, 0, X > {
    enum { VALUE = meta::Number< 0x1f >::VALUE };
  };

  template< size_t T, size_t B, BerTagType TYP, BerTagClass CLS > 
  struct TagByte {
    enum{ INDEX = ((B + 1) - TagLength< T >::VALUE) };
    enum{ RVALUE = meta::IF< ((T < 0x1f) || (INDEX < 0)),
	    _STagByte<T,B>, _LTagByte<T,B,(int)INDEX> >::RESULT::VALUE };
    enum{ VALUE = meta::IF< B == 0, 
	    _TagVal<RVALUE, TYP, CLS>,
	    meta::Number<RVALUE> >::RESULT::VALUE };
  };

  template< size_t T, size_t B, BerTagType TYP, BerTagClass CLS > 
  class _SetTagVal {
  public:
    static inline void exec(unsigned char *A) {  
      A[B] = TagByte<T,B,TYP,CLS>::VALUE;
      _SetTagVal<T, B-1,TYP,CLS>::exec(A);
    }  
  };
  template< size_t T, BerTagType TYP, BerTagClass CLS > 
  class _SetTagVal<T,0,TYP,CLS> {
  public:
    static inline void exec(unsigned char *A) {  
      A[0] = TagByte<T,0,TYP,CLS>::VALUE;
    }  
  };

  template< size_t T, size_t B, BerTagType TYP, BerTagClass CLS > 
  class _CmpTagVal {
  public:
    static inline bool exec(const unsigned char *A) {  
      if(A[B] == TagByte<T,B,TYP,CLS>::VALUE) {
	return _CmpTagVal<T, B-1,TYP,CLS>::exec(A);
      } else {
	return false;
      }
    }  
  };
  template< size_t T, BerTagType TYP, BerTagClass CLS > 
  class _CmpTagVal<T,0,TYP,CLS> {
  public:
    static inline bool exec(const unsigned char *A) {  
      return A[0] == TagByte<T,0,TYP,CLS>::VALUE;
    }  
  };


  template< size_t T, BerTagType TYP, BerTagClass CLS > 
  class TagString {
  public:
    enum { SIZE = TagLength<T>::VALUE };
    enum { VALUE = T };
    enum { TYPE = TYP };
    enum { CLASS = CLS };
    
    static void write(unsigned char *tag) {
      _SetTagVal<T, SIZE - 1, TYP, CLS >::exec(tag);
    }

    static bool isEqual(const unsigned char *tag) {
      return _CmpTagVal<T, SIZE - 1, TYP, CLS >::exec(tag);
    }
  };

  template< size_t S, size_t T, BerTagType TYP, BerTagClass CLS > 
  class __TagArray {
  public:
    enum { SIZE = S };
    enum { VALUE = T };
    enum { TYPE = TYP };
    enum { CLASS = CLS };
    static bool isEqual(const unsigned char *tag) {
      return _CmpTagVal<T, SIZE - 1, TYP, CLS >::exec(tag);
    }
    typedef const unsigned char ctyp;
  };

#define _TagArray_OPS \
    const unsigned char operator[](size_t& i) const {\
      return (i < __TagArray<0,T,TYP,CLS>::SIZE)? (&a0)[i] : 0;                 \
    }                                                \
    const unsigned char *operator()() const {        \
      return &a0;                                    \
    }


  template< size_t S, size_t T, BerTagType TYP, BerTagClass CLS > 
  class _TagArray : public __TagArray<0,T,TYP,CLS> {
  public:
    _TagArray() { mgrThrow(ERR_INT_IMP); }
    const unsigned char operator[](size_t& i) const {
      return 0;
    }    
    const unsigned char *operator()() const { return NULL; }
  };

  template< size_t T, BerTagType TYP, BerTagClass CLS > 
  class _TagArray<1, T, TYP, CLS >  : public __TagArray<1,T,TYP,CLS> {
  private:
    const unsigned char a0;
  public:
    _TagArray() : a0(TagByte<T, 0, TYP, CLS>::VALUE) {}
    _TagArray_OPS
  };

  template< size_t T, BerTagType TYP, BerTagClass CLS > 
  class _TagArray<2, T, TYP, CLS >  : public __TagArray<2,T,TYP,CLS> {
  private:
    const unsigned char a0, a1;
  public:
    _TagArray() : a0(TagByte<T, 0, TYP, CLS>::VALUE),
		  a1(TagByte<T, 1, TYP, CLS>::VALUE) {}
    _TagArray_OPS
  };

  template< size_t T, BerTagType TYP, BerTagClass CLS > 
  class _TagArray<3, T, TYP, CLS >  : public __TagArray<3,T,TYP,CLS> {
  private:
    const unsigned char a0, a1, a2;
  public:
    _TagArray() : a0(TagByte<T, 0, TYP, CLS>::VALUE),
		  a1(TagByte<T, 1, TYP, CLS>::VALUE),
		  a2(TagByte<T, 2, TYP, CLS>::VALUE) {}
    _TagArray_OPS
  };

  template< size_t T, BerTagType TYP, BerTagClass CLS > 
  class _TagArray<4, T, TYP, CLS >  : public __TagArray<4,T,TYP,CLS> {
  private:
    const unsigned char a0, a1, a2, a3;
  public:
    _TagArray() : a0(TagByte<T, 0, TYP, CLS>::VALUE),
		  a1(TagByte<T, 1, TYP, CLS>::VALUE),
		  a2(TagByte<T, 2, TYP, CLS>::VALUE),
		  a3(TagByte<T, 3, TYP, CLS>::VALUE) {}
    _TagArray_OPS
  };

  template< size_t T, BerTagType TYP, BerTagClass CLS > 
  class _TagArray<5, T, TYP, CLS >  : public __TagArray<5,T,TYP,CLS> {
  private:
    const unsigned char a0, a1, a2, a3, a4;
  public:
    _TagArray() : a0(TagByte<T, 0, TYP, CLS>::VALUE),
		  a1(TagByte<T, 1, TYP, CLS>::VALUE),
		  a2(TagByte<T, 2, TYP, CLS>::VALUE),
		  a3(TagByte<T, 3, TYP, CLS>::VALUE),
		  a4(TagByte<T, 4, TYP, CLS>::VALUE) {}
    _TagArray_OPS
  };

#undef _TagArray_OPS

  template< size_t T, BerTagType TYP, BerTagClass CLS > 
  class TagArray{
  public:
    typedef _TagArray< TagLength< T >::VALUE, T, TYP, CLS > RESULT;
  };

# endif // _TLV_BERTREE_META_H_
#endif
