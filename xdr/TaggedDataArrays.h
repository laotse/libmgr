/*
 *
 * Tagged Data File as an exchange standard for binary data,
 * which draws on BER for tagging,
 * on ASN.1 where applicable, and
 * on RFC 1832 - Sun XDR for some storage formats
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: TaggedDataArrays.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *
 * TaggedDataFile - class for organisation of data into scopes
 *
 * This defines the values:
 *
 */

#ifndef _XDR_TAGGEDDATAARRAYS_H_
# define _XDR_TAGGEDDATAARRAYS_H_

#include <TaggedDataFile.h>

namespace mgr {

/*
 * Don't touch this one, unless you definitely know what you're doing
 *
 */
class _TDArray : public TDFItem {
public:
  typedef BerContentTag::TagString<(int)TaggedDataFile::INT_ARRAY,
    BerContentTag::BER_CONSTRUCTED, BerContentTag::BER_APPLICATION> tIntArray;
  static const unsigned char bIntArray[tIntArray::SIZE];
  typedef BerContentTag::TagString<(int)TaggedDataFile::DOUBLE_ARRAY,
    BerContentTag::BER_CONSTRUCTED, 
    BerContentTag::BER_APPLICATION> tDoubleArray;
  static const unsigned char bDoubleArray[tDoubleArray::SIZE];

protected:
  // array core
  BerTree *arrayCoreWrite(const BerContentTag& mtag, const _wtBuffer& c, m_error_t& err) const;
  m_error_t arrayCoreRead(const BerContentTag& mtag, const BerTag *arry, 
			    _wtBuffer& c);

  BerTree *writeTagCore(const BerContentTag& mtag, 
			const _wtBuffer& A, m_error_t *err) const;


public:
  _TDArray(TaggedDataFile::AppTags tag) : 
    TDFItem((size_t)tag, 
	    BerContentTag::BER_CONSTRUCTED, 
	    BerContentTag::BER_APPLICATION) {}
  virtual ~_TDArray() {}
  const char *VersionTag(void) const;

  virtual BerTree *writeTag(m_error_t *err = NULL) const = 0;
  virtual m_error_t readTag(const BerTag& t) = 0;
  virtual m_error_t dump(FILE *f = stdout, const char *prefix = NULL) const = 0;  

};

/*
 * The Array template:
 *
 * BASE: Storage type in TaggedDataFile
 *    T: Tag used in TaggedDataFile
 *
 */
template<typename BASE, TaggedDataFile::AppTags T>
class TDArray : public _TDArray {
protected:
  wtBuffer<BASE> A;

public:
  template<typename OUT>
  m_error_t import(const OUT *d, const size_t& s){
    if(!d) return ERR_PARAM_NULL;
    m_error_t res = A.allocate(s * sizeof(BASE));
    if(res != ERR_NO_ERROR) return res;    
    BASE *b = A.writePtr();
    if(b){
      for(size_t i=0;i<s;i++){
	// GCC automagically selects the correct template function
	// ASN1IO inherits xdrIO
	XDR::xdrWrite(b++, static_cast<BASE>(*d++));
      }
    } else res = ERR_INT_STATE;
    return res;
  }

  template<typename OUT>
  m_error_t get(OUT *d){
    if(!d) return ERR_PARAM_NULL;
    const BASE *b = A.readPtr();
    if(!b) return ERR_INT_STATE;
    for(size_t i=0; i<A.size(); i++){
      *d++ = static_cast<OUT>(XDR::xdrRead<BASE>(b++));
    }
    return ERR_NO_ERROR;
  }

  TDArray() : _TDArray(T) {}
  template<typename OUT>
  TDArray(const OUT *d, const size_t& s) : _TDArray(T){
    m_error_t res = import(d,s);
    if(res != ERR_NO_ERROR) mgrThrow(res);
  }

  virtual ~TDArray() {}

  size_t size(void){
    return A.size();
  }

  virtual BerTree *writeTag(m_error_t *err = NULL) const {
    return writeTagCore(Tag, A, err);
  }
  virtual m_error_t readTag(const BerTag& arry){
    return _TDArray::arrayCoreRead(Tag, &arry, A);
  }
  m_error_t readTag(const BerTag* arry){
    if(!arry) return ERR_PARAM_NULL;
    return _TDArray::arrayCoreRead(Tag, arry, A);
  }

  virtual m_error_t dump(FILE *f = stdout, const char *prefix = NULL) const;
};

typedef TDArray<XDR::Int, TaggedDataFile::INT_ARRAY> TDIntArray;
typedef TDArray<XDR::Short, TaggedDataFile::INT_ARRAY> TDShortArray;
typedef TDArray<XDR::Double, TaggedDataFile::DOUBLE_ARRAY> TDDoubleArray;

}; // namespace mgr

#endif // _XDR_TAGGEDDATAARRAYS_H_
