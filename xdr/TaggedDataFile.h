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
 * $Id: TaggedDataFile.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *
 * TaggedDataFile - class for organisation of data into scopes
 *
 * This defines the values:
 *
 */

#ifndef _XDR_TAGGEDDATAFILE_H_
# define _XDR_TAGGEDDATAFILE_H_

#include <BerTree.h>
#include <asn1IO.h>

namespace mgr {

// item interface
class TDFItem {
protected:
  ASN1IO asn;
  const BerContentTag Tag;

  // prepare and clean-up local scope for writeTag methods
  BerTree *initWrite(BerTag *& tg, m_error_t *&err) const;
  void finishWrite(BerTree*& tr, BerTag*& tg, 
		   m_error_t*& err, m_error_t& ierr) const;


public:
  TDFItem(const size_t& number, const BerContentTag::BerTagType& ty, const BerContentTag::BerTagClass& cl) : Tag(number,ty,cl) {};
  virtual ~TDFItem();
  virtual BerTree *writeTag(m_error_t *err = NULL) const = 0;
  virtual m_error_t readTag(const BerTag& t) = 0;
  virtual m_error_t dump(FILE *f = stdout, const char *prefix = NULL) const = 0;  
  inline const BerContentTag& tag(void) const{
    return Tag;
  }
};

class TaggedDataFile {
public:
  // class application
  enum DataTags {
    VOID = 0x00,
    HEADER = 0x04,
    INT_ARRAY = 0x10, 
    DOUBLE_ARRAY = 0x11
  };
  typedef enum DataTags AppTags;

  // tags used
  typedef BerContentTag::TagString<(int)HEADER,
    BerContentTag::BER_CONSTRUCTED, BerContentTag::BER_APPLICATION> tHeader;
  static const unsigned char bHeader[tHeader::SIZE];
  typedef BerContentTag::TagString<(int)INT_ARRAY,
    BerContentTag::BER_CONSTRUCTED, BerContentTag::BER_APPLICATION> tIntArray;
  static const unsigned char bIntArray[tIntArray::SIZE];
  typedef BerContentTag::TagString<(int)DOUBLE_ARRAY,
    BerContentTag::BER_CONSTRUCTED, BerContentTag::BER_APPLICATION> tDoubleArray;
  static const unsigned char bDoubleArray[tDoubleArray::SIZE];

protected:
  ASN1IO asn;
  BerTree ber;
  bool newScope;
  
public:
  TaggedDataFile() : newScope(false) {};

  void reset(void){
    ber.clear();
    newScope = false;
  }
  

  // Write Access
  m_error_t openScope(const BerContentTag& t);
  const BerContentTag* closeScope();
  // This assumes that t is allocated by new
  m_error_t addTag(BerTag* t);
  // These ones copy the data
  m_error_t addTag(const BerTree& tr);
  inline m_error_t addTag(BerTag& t){
    BerTree tr(&t);
    return addTag(tr);
  }

  // Item Interface
  m_error_t addItem(const TDFItem& it);
  m_error_t readItem(TDFItem& it, size_t offset = 0, bool absolute = true);
  inline bool readOk(m_error_t& e){
    return ((e == ERR_NO_ERROR) || (e == ERR_CANCEL));
  }

  inline bool readFinal(m_error_t& e){
    return (e == ERR_CANCEL);
  }

  // Read Access
  inline void rewind(void){
    ber.root();
    newScope = false;
  }
  m_error_t enterScope(const BerContentTag& t, size_t offset = 0, bool absolute = true);
  BerTree *getScope(void);

  // Serialize
  m_error_t write(StreamDump& s);
  inline m_error_t read(const void *b, const size_t& s){
    return ber.replace((const unsigned char *)b,s,true);
  }

  inline m_error_t read(const _wtBuffer& s){
    return read(s.rawPtr(), s.byte_size());
  }

  /*
  inline m_error_t read(const wtBuffer<unsigned char>& s){
    return read(s.readPtr(), s.byte_size());
  }
  inline m_error_t read(const wtBuffer<char>& s){
    return read(s.readPtr(), s.byte_size());
  }
  */  

  const char *VersionTag(void) const;
};

class TDFDataHeader : public TDFItem {
protected:
  const char *Name;
  const char *Author;
  time_t Date;
  // not const, can be changed by readTag()
  int Type;

private:
  m_error_t init(const char *n, const char *auth);

public:
  virtual BerTree *writeTag(m_error_t *err = NULL) const;
  virtual m_error_t readTag(const BerTag& t);
  virtual m_error_t dump(FILE *f = stdout, const char *prefix = NULL) const;

  TDFDataHeader(const char *n = NULL, const char *auth = NULL) : 
    TDFItem((size_t)TaggedDataFile::HEADER, 
	    BerContentTag::BER_CONSTRUCTED, 
	    BerContentTag::BER_APPLICATION), Type(1) {

    m_error_t res = init(n,auth);
    if(res != ERR_NO_ERROR) mgrThrow(res);
  }

  TDFDataHeader(const BerTag& t) :
    TDFItem((size_t)TaggedDataFile::HEADER, 
	    BerContentTag::BER_CONSTRUCTED, 
	    BerContentTag::BER_APPLICATION), Type(1) {

    m_error_t res = init(NULL,NULL);
    if(res != ERR_NO_ERROR) mgrThrow(res);
    res = readTag(t);
    if(res != ERR_NO_ERROR) mgrThrow(res);
  }

  virtual ~TDFDataHeader() {
    if(Name) ::free(const_cast<char *>(Name));
    if(Author) ::free(const_cast<char *>(Author));
  }

  inline const time_t& date(void) const {
    return Date;
  }
  inline void date(const time_t& t) {
    Date = t;
  }

  inline const char *name(void) const {
    return Name;
  }
  m_error_t name(const char *n);

  inline const char *author(void) const {
    return Author;
  }
  m_error_t author(const char *n);

  const char * VersionTag(void) const;
};

}; // namespace mgr

#endif // _XDR_TAGGEDDATAFILE_H_
