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
 * $Id: asn1IO.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *
 * TaggedDataFile - class for organisation of data into scopes
 *
 * This defines the values:
 *
 */

#ifndef _XDR_ASN1IO_H_
# define _XDR_ASN1IO_H_

#include <time.h>
#include <BerTree.h>
#include <xdrOrder.h>

namespace mgr {

class ASN1IO {
public:
  // Universal tags from ITU-T X.680 (07/2002)
  enum ASN1Tags {
    ASN1_BOOLEAN = 1,
    ASN1_INTEGER      = 2,
    ASN1_BIT_STRING = 3,
    ASN1_OCTET_STRING = 4,
    ASN1_NULL = 5,
    ASN1_OID = 6,
    ASN1_OBJECT_DESCRIPTOR = 7,
    ASN1_INSTANCE = 8,
    ASN1_REAL = 9,
    ASN1_ENUMERATE = 10,
    ASN1_EMBEDDED_PDV = 11,
    ASN1_UTF8_STRING = 12,
    ASN1_REL_OID = 13,
    ASN1_SEQ = 0x10,
    ASN1_SET = 0x11,
    ASN1_PRINTABLE_STRING = 0x13,
    ASN1_T61_STRING = 0x14,	
    ASN1_IA5_STRING = 0x16,		
    ASN1_UTC_TIME = 0x17	
  };
  typedef enum ASN1Tags ASN1Tag;

  // tags used
  typedef BerContentTag::TagString<(int)ASN1_INTEGER,
    BerContentTag::BER_PRIMITIVE, BerContentTag::BER_UNIVERSAL> intTag;
  typedef BerContentTag::TagString<(int)ASN1_IA5_STRING,
    BerContentTag::BER_PRIMITIVE, BerContentTag::BER_UNIVERSAL> ia5Tag;
  typedef BerContentTag::TagString<(int)ASN1_PRINTABLE_STRING,
    BerContentTag::BER_PRIMITIVE, BerContentTag::BER_UNIVERSAL> printTag;
  typedef BerContentTag::TagString<(int)ASN1_UTC_TIME,
    BerContentTag::BER_PRIMITIVE, BerContentTag::BER_UNIVERSAL> utcTag;

 protected:
  static const unsigned char asn1_int[intTag::SIZE];
  static const unsigned char asn1_ia5[ia5Tag::SIZE];
  static const unsigned char asn1_print[printTag::SIZE];
  static const unsigned char asn1_utc[utcTag::SIZE];

 public:
  template<typename T> m_error_t readInt(const BerTag& tag, T& val) const {
    union { T i; unsigned char c[sizeof(T)]; } u;
    if(!intTag::isEqual(tag.tag().readPtr())) return ERR_PARAM_TYP;
    size_t cl = tag.c_size();
    const unsigned char *c = tag.content().readPtr();
    if(!cl || !c) return ERR_PARAM_UDEF;
    const size_t l = sizeof(T);
    while(cl > l) 
      if(*c++ != 0) return ERR_PARAM_RANG; else cl--;
    if(cl < l){
      u.i = 0;
      for(size_t i=l-cl;i<l;i++) u.c[i] = *c++;
      c = u.c;
    }
    val = XDR::xdrRead<T>(c);
    return ERR_NO_ERROR;
  }
    
  template<typename T> BerTag *writeIntPacked(const T& val) const {
    BerTag *tag = new BerTag(asn1_int,intTag::SIZE);
    if(!tag) return tag;
    unsigned char cb[sizeof(T)];
    if(val){
      XDR::xdrWrite<T>(cb,val);
      const unsigned char *c = cb;
      size_t i=sizeof(T); 
      for(;(i>0) && (*c == 0);i--,c++);
      tag->content(c,i);
    } else {
      cb[0] = 0;
      tag->content(cb,1);
    }
    if(ERR_NO_ERROR != tag->detach()){
      delete tag;
      return NULL;
    }
    return tag;    
  }

  template<typename T> BerTag *writeInt(const T& val) const {
    BerTag *tag = new BerTag(asn1_int,intTag::SIZE);
    if(!tag) return tag;
    unsigned char *c = tag->allocate(sizeof(T));
    if(!c){
      delete tag;
      return NULL;
    }
    XDR::xdrWrite<T>(c,val);

    return tag;
  }

  m_error_t readPrintableString(const BerTag& tag, char *s, const size_t& l) const;
  BerTag *writePrintableString(const char *s, m_error_t *err = NULL) const;

  m_error_t readIA5String(const BerTag& tag, char *s, const size_t& l) const;
  BerTag *writeIA5String(const char *s, m_error_t *err = NULL) const;

  m_error_t readUTC(const BerTag& tag, struct tm *tm);
  BerTag *writeUTC(const struct tm *tm = NULL, m_error_t *err = NULL) const;
  inline BerTag *writeUTC(const time_t& t, m_error_t *err = NULL) const {
    struct tm ts;
    localtime_r(&t,&ts);
    return writeUTC(&ts, err);
  }

  const char * VersionTag(void) const;
};

}; // namespace mgr
#endif // _XDR_ASN1IO_H_
