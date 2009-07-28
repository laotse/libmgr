/*
 *
 * BER, DER, TLV, ASN.1, ... Trees
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: BerTree.cpp,v 1.6 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  BerTag  - node for a TAG
 *  BerTree - the XTree of berTag
 *
 * This defines the values:
 *
 */

#include "BerTree.h"
#include "BerTree.tag"

#define DEBUG_MARK 1
#define DEBUG_DUMP 2

#define DEBUG (DEBUG_MARK | DEBUG_DUMP)
#ifdef DEBUG
# include <stdio.h>
# include <HexDump.h>
#endif
#include <mgrDebug.h>

using namespace mgr;

#ifdef DEBUG
# ifdef EXT_DEBUG
extern FileDump dbgStream;
extern HexDump dbgHex;
# else
FileDump dbgStream(DEBUG_LOG);
HexDump dbgHex(&dbgStream);
# endif
#endif

/*
 * Content Dumper
 *
 */

ssize_t BerContentRegion::dump(char *s, const size_t& l) const {
  const unsigned char *d = get_fix();
  ssize_t written = 0;
  size_t read = length;

  if(!read){
    *s = 0;
    return written;
  }
  if(!d){
    // dummy length
    const char *dtxt = "(dummy)";
    if(l>strlen(dtxt)) {
      memcpy(s,dtxt,strlen(dtxt)+1);
    }
    return 0;
  }
  while(read && (written < static_cast<ssize_t>(l-4))){
    if(written){
      *s++ = ' ';
      written++;
    }
    unsigned char c = *d >> 4;
    c += (c<10)? '0' : 'a'-10;
    *s++ = c;
    written++;
    c = *d++ & 0x0f;
    read--;
    c += (c<10)? '0' : 'a'-10;
    *s++ = c;
    written++;      
  }
  *s=0;
  return length-read;
}

// this has become obsolete
m_error_t BerContentRegion::own(_wtBuffer& b){
  _wtBuffer::operator=(b);
  b.free();

  return ERR_NO_ERROR;
}

/*
 * TAG parser
 *
 */

#define BER_TYPE_SHIFT BerContentTag::type_shift
#define BER_CLASS_SHIFT BerContentTag::class_shift

inline BerContentTag::BerTagType cast_byte_type(const unsigned char& b){
  if(b >> BER_TYPE_SHIFT & 1) return BerContentTag::BER_CONSTRUCTED;
  return BerContentTag::BER_PRIMITIVE;
}

BerContentTag::BerTagType BerContentTag::Type(void) const {
  const unsigned char *c = get_fix();
  if(!c) mgrThrow(ERR_PARAM_NULL);
  return cast_byte_type(*c);
}

BerContentTag::BerTagClass BerContentTag::Class(void) const {
  const unsigned char *c = get_fix();
  if(!c) mgrThrow(ERR_PARAM_NULL);
  return static_cast<BerTagClass>((*c >> BER_CLASS_SHIFT) & 3);
}

m_error_t BerContentTag::replace(const unsigned char * const d, const size_t & l){
  if(!d || !l) return ERR_PARAM_NULL;
    
  unsigned char current = *d;    
  free();
  buf.fix = d;
  length = 1;
  if((current & 0x1f) == 0x1f){
    // adding more bytes
    do {
      if(length >= l){
	buf.fix = NULL;
	length = 0;
	return ERR_PARAM_RANG;
      }
    }while(d[length++] & 0x80);
  }
  return ERR_NO_ERROR;
}

m_error_t BerContentTag::replace(const size_t& number, const BerTagType& ty, const BerTagClass& cl){
  unsigned char tid;
  
  if(ty > 1) return ERR_INT_RANG;
  if(cl > 3) return ERR_INT_RANG;
  if(number < 0x1f){
    tid = number & 0x1f;
  } else {
    tid = 0x1f;
  }
  tid |= ty << BER_TYPE_SHIFT;
  tid |= cl << BER_CLASS_SHIFT;    

  if(number < 0x1f){
    allocate(1);
    m_error_t err = ERR_NO_ERROR;
    get_var( err )[0] = tid;
    return err;
  }
 
  int i=0;
  for(size_t q = number;q;q>>=1,i++); 
  i += 5;     // +6, -1
  i /= 7;      // divide by 7 bit, round up 
  allocate(i+1);
  m_error_t err = ERR_NO_ERROR;
  unsigned char *d = get_var( err );
  if(err != ERR_NO_ERROR) return err;
  d[0] = tid;
  d[i--] = static_cast<unsigned char>(number & 0x7f);
  for(size_t q = number >> 7;i;i--){
    d[i] = static_cast<unsigned char>((q & 0x7f) | 0x80);
    q >>= 7;
  }
  
  return ERR_NO_ERROR;
}

/*
 * Length parser
 *
 */

/*! Reads the length from the given location according to the
    Ber length field coding. If the length extends beyond
    the valid buffer length ERR_PARAM_LEN is returned. If
    the length exceeds the value range of size_t 
    ERR_PARAM_RANG is returned.
*/
m_error_t BerContentLength::replace(const unsigned char * const d, const size_t & l){
  const size_t msb = 0xff << ((sizeof(size_t) - 1) * 8);
  if(!d || !l) return ERR_PARAM_NULL;
    
  unsigned char current = *d;    
  free();
  buf.fix = d;
  length = 1;

  xpdbg(DUMP,"Length ID: 0x%x\n",(int)current);

  if(!(current & 0x80)){
    val = current;
    return ERR_NO_ERROR;
  }
    
  current &= 0x7f;
  if(length + current > l) return ERR_PARAM_LEN;
    
  for(int i=0;i<current;i++){
    if(i){
      if(val & msb){
	val = 0;
	length = 0;
	buf.fix = NULL;
	return ERR_PARAM_RANG;
      }
      val <<= 8;
    } else val = 0;
    val += d[i+1];
  }
  length += current;

  return ERR_NO_ERROR;
}

/*! \param l Length to set
    \return Error code as defined in mgrError.h

    This function set the val field and also synthesizes
    a BER coded length in the _wtBuffer.
*/
m_error_t BerContentLength::value(const size_t& l){
  m_error_t err = ERR_NO_ERROR;
  val = l;	
  if(l < 0x80){
    allocate(1);
    get_var( err )[0] = static_cast<unsigned char>(l & 0x7f);
  } else {	    
    int i=0;
    for(size_t q = l;q;q>>=1,i++); 
    i += 6;     // this is +7 and -1 from the for loop
    i >>= 3;  // i /= 8
    allocate(i+1);
    unsigned char *d = get_var( err );
    d[0] = static_cast<unsigned char>((i & 0x7f) | 0x80);
    for(size_t q = l;i;i--){
      d[i] = static_cast<unsigned char>(q & 0xff);
      q >>= 8;
    }
  }
  
  return err;
}

/*
 *  TLV parser
 *
 */

m_error_t BerTag::readBer(const unsigned char * const d, const size_t & l){
  const unsigned char * s = d;
  size_t remain = l;
    
  if(!s || !l) return ERR_PARAM_NULL;
    
  clear();

  xpdbg(MARK,"Start reading Tag from %.2x remain %u\n",(int)*s,(unsigned int)remain);    
  m_error_t error = Tag.replace(s,remain);
  if(error != ERR_NO_ERROR) return error;    
  s += Tag.byte_size();
  remain -= Tag.byte_size();

  xpdbg(MARK,"Start reading Length from %.2x remain %u\n",(int)*s,(unsigned int)remain);
  if(!remain) return ERR_CANCEL;
  error = Length.replace(s,remain);
  if(error != ERR_NO_ERROR){
    Tag.free();
    return error;
  }    
  s += Length.byte_size();
  remain -= Length.byte_size();
  if(Length.value() > remain){
    Tag.free();
    Length.free();
    return ERR_PARAM_RANG;
  }

  xpdbg(MARK,"Start reading Value from %.2x remain %u\n",(int)*s,(unsigned int)remain);    
  Value.replace(s,Length.value());

#if DEBUG_CHECK(DUMP)
  dump(DEBUG_LOG,"### ");
#endif
    
  return ERR_NO_ERROR;
}

m_error_t BerTag::recalcSize(size_t &s, const bool follow){    
  m_error_t error;

  if(Tag.Type() == BerContentTag::BER_CONSTRUCTED){
    size_t cs = 0;    
    if(this->child){
      error = static_cast<class BerTag *>(child)->recalcSize(cs,true);
      if(error != ERR_NO_ERROR) return error;
      error = content(cs);
    } else {
      error = content(0);
    }
    if(error != ERR_NO_ERROR) return error;
  } else {
    // PRIMITIVE Tags must not have children or dummy length 
    if(child) return ERR_PARS_STX;
    if(Length.value() && !Value.readPtr()) return ERR_PARAM_NULL;
  }
  // TLV overhead + contents
  s += Tag.size() + Length.size()  + Value.size();
  
  if(follow){
    class BerTag *n = static_cast<class BerTag *>(this->next);
    while(n){
      m_error_t error = n->recalcSize(s,false);
      if(error != ERR_NO_ERROR) return error;
      n = static_cast<class BerTag *>(n->next);
    }
  }
  
  return ERR_NO_ERROR;
}

void BerTag::clear(void){
  Tag.free();
  Length.free();
  Value.free();
}

const char * BerTag::VersionTag(void) const {
  return _VERSION_;
}

m_error_t BerTag::write(StreamDump& s) const {
  m_error_t err = Tag.write(s);
  if(err != ERR_NO_ERROR) return err;
  err = Length.write(s);
  if(err != ERR_NO_ERROR) return err;
  if(Length.value() && Value.readPtr())
    err = Value.write(s);

  return err;
}

ssize_t BerTag::dump(FILE *f, const char *prefix) const{
  wtBuffer<char> out;  

  if(!prefix) prefix = "";

  m_error_t err = out.trunc(Tag.byte_size() * 3 + 10,true);
  //xpdbg(MARK,"Going to dump (0x%.3x)\n",(int)err);
  xpdbg(DUMP,"TAG: %p (%d), LEN: %p (%d), VAL: %p (%d)\n",
	Tag.readPtr(), Tag.byte_size(),
	Length.readPtr(), Length.byte_size(),
	Value.readPtr(), Value.byte_size());
  if(err != ERR_NO_ERROR) return -1;
  Tag.dump(out.writePtr(),out.size());
  ssize_t wr = fprintf(f,"%sTAG: %s\n",prefix,out.readPtr());
  if(wr < 0) return wr;

  err = out.trunc(Length.byte_size() * 3 + 10,true);
  if(err != ERR_NO_ERROR) return -1;
  Length.dump(out.writePtr(),out.size());
  ssize_t w = fprintf(f,"%sLEN (%u): %s\n",prefix,Length.value(),out.readPtr());
  if(w < 0) return w;
  wr += w;

  err = out.trunc(Value.byte_size() * 3 + 10,true);
  if(err != ERR_NO_ERROR) return -1;
  Value.dump(out.writePtr(),out.size());
  w = fprintf(f,"%sVAL (%u): %s\n",prefix,Value.byte_size(),out.readPtr());
  if(w < 0) return w;
  wr += w;

  return wr;
}

/*
 * The BerTree stuff
 *
 */

BerTree::~BerTree(){
  if(ownNodes){
    remove(sroot,true);
  }
}

void BerTree::freeNode(HTreeNode *n) const {
  BerTag *t = static_cast<BerTag *>(n);
#if DEBUG_CHECK(DUMP)
  t->dump(DEBUG_LOG,"###~ ");
#endif    
  t->clear();
}


void BerTree::clear(void){
  if(ownNodes){
    remove(sroot,true);
  }
  ownNodes = false;
  garbage = NULL;
  input.free();
  XTree<BerTag>::clear();
}

BerTree& BerTree::operator=(const BerTree& t){
  XTree<class BerTag>::operator=(t);
  garbage = t.garbage;
  input.free();
  ownNodes = false;
  
  return *this;
}


m_error_t BerTree::parseContent(BerTag *c) const {
  if(c->tag().Type() == BerContentTag::BER_PRIMITIVE) return ERR_NO_ERROR;
  size_t l = 0;
  BerTag *p = c, *t;
  m_error_t err = ERR_NO_ERROR;
  while(l < c->c_size()){
    t = new BerTag;
    if(!t){
      err = ERR_MEM_AVAIL;
      break;
    }
#if DEBUG_CHECK(DUMP)
    size_t dbg_l = 0;
    dbgHex.textf(&dbg_l,"### Subparsing (%p): ",c->content().readPtr()+l);
    dbg_l = c->c_size()-l;
    dbgHex.write(c->content().readPtr()+l,&dbg_l);
    dbgHex.lineFeed();
#endif
    err = t->readBer(c->content().readPtr()+l,c->c_size()-l);
    if(err != ERR_NO_ERROR){
      // t is loose, delete it
      delete t;
      t = NULL;
      break;
    }
    if(p == c){
      insertChild(c,t);
    } else {
      insertNext(p,t);
    }
    err = parseContent(t);
    if(err != ERR_NO_ERROR) break;
    p = t;
    l += p->size();
  }
  if(err != ERR_NO_ERROR){
    t = static_cast<BerTag *>(c->getChild());
    while(t){     
      p = static_cast<BerTag *>(t->getNext());
      freeNode(t);
      t = p;
    }
    clearChild(c);
  } else {
    c->content(c->c_size());
  }

  return err;
}

m_error_t BerTree::replace(const unsigned char *data, size_t l, bool copy){
  m_error_t error;
  class BerTag *t;
  size_t read;

  remove(root(), ownNodes); // remove is sane for NULL pointers
  ownNodes = true;
  error = input.replace(data,l);
  if(error != ERR_NO_ERROR) return error;
  if(copy){
    error = input.branch();
    if(error != ERR_NO_ERROR) return error;
  }
  t = new BerTag;
  if(!t) return ERR_MEM_AVAIL;
  do {
    error = t->readBer(input.readPtr(),input.byte_size());
    if(error != ERR_NO_ERROR) break;
    initTree(t);
    read = t->size();
    error = parseContent(t);
    if(error != ERR_NO_ERROR) break;
    while(read < l){
      t = new BerTag;
      if(!t){
	error = ERR_MEM_AVAIL;
	break;
      }
      error = t->readBer(input.readPtr()+read,l-read);
      if(error != ERR_NO_ERROR){
	// FIXME: Some errors might not be due to garbage trailing
	garbage = input.readPtr()+read;
	error = ERR_NO_ERROR;
	break;
      }
      error = parseContent(t);
      if(error != ERR_NO_ERROR){
	// oops, this node is rotten
	// do not attach - is garbage
	break;
      }
      appendNext(t,true);
      read += t->size();
      t = NULL;
    }
  } while(0);
  if(t) delete t;
  if(error != ERR_NO_ERROR) {}
  
  return error;
}

m_error_t BerTree::write(StreamDump& s, bool calc) {
  BerTag *c = current();
  m_error_t err = ERR_NO_ERROR;
  size_t res = 0;
  int lvl = depth();

  if(calc){
    err = c->recalcSize(res,true);
    if(err != ERR_NO_ERROR) return err;
  }
  while(c){
    err = c->write(s);
    if(err != ERR_NO_ERROR) return err;
    c = iterate(&lvl);
  }

  return err;
}

ssize_t BerTree::dump(FILE *f, const char *prefix){
  ssize_t written = 0, res = 0;
  int lvl = depth();
  BerTag *c = current();
  char pbuf[128];
  const char *mpre = "";
  
  if(!c) return 0;
  if(!prefix) prefix = mpre;
  // the cygwin version of g++ does not accept it otherwise
  size_t sres = 0;
  if(ERR_NO_ERROR != c->recalcSize(sres,true)) return -3;
  res = static_cast<ssize_t>(sres);
  while(c){
    sprintf(pbuf, "%s%4d: ",prefix,lvl);
    res = c->dump(f,pbuf);
    if(res < 0) return res;
    written += res;
    c = iterate(&lvl);
  }
  
  return written;
}  

/*
 * find(), etc.
 *
 */

BerTag *BerTree::find(const BerContentTag& t) const{
  BerTree b(current());

  BerTag *c = b.root();
  while(c){
    if(c->tag() == t) return c;
    c = b.iterate();
  }
  
  return NULL;
}

BerTag *BerTree::find(const unsigned char *data, const size_t& s, m_error_t *err) const {
  BerContentTag t;

  m_error_t ierr = t.replace(data,s);
  if(err) *err = ierr;
  if(ierr != ERR_NO_ERROR) return NULL;
  return find(t);
}

/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <stdio.h>
#include <HexDump.h>
#include <wtBufferDump.h>

int main(int argc, char *argv[]){
  m_error_t res;
  size_t tests, errors;

  tests = errors = 0;

  // TAG 0e, LEN 2 byte, Content: a5 ff
  const char *stag = "\x0e\x02\xa5\xff";
  BerTag temp;

  printf("Test %d: Read simple TAG\n",++tests);
  res = temp.readBer((const unsigned char *)stag, strlen(stag));
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: readBer() failed 0x%.4x\n",(int)res);
  } else {
    temp.dump(stdout,"??? ");
    puts("+++ readBer() finished OK!");
  }

  // TAG 0e, LEN 2 byte, Content: a5 ff
  const char *ctag = "\x1f\x0e\x81\x02\xa5\xff";
  printf("Test %d: Read complex TAG\n",++tests);
  res = temp.readBer((const unsigned char *)ctag, strlen(ctag));
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: readBer() failed 0x%.4x\n",(int)res);
  } else {
    temp.dump(stdout,"??? ");
    puts("+++ readBer() finished OK!");
  }

  // create a TLV
  temp.clear();
  printf("Test %d: Create TAG\n",++tests);
  res = temp.tag(0x42,BerContentTag::BER_PRIMITIVE,BerContentTag::BER_PRIVATE);
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: tag() failed 0x%.4x\n",(int)res);
  } else {
    temp.dump(stdout,"??? ");
    puts("+++ readBer() finished OK!");
  }

  printf("Test %d: Add some contents\n",++tests);
  res = temp.content((const unsigned char *)ctag, strlen(ctag));
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: content() failed 0x%.4x\n",(int)res);
  } else {
    temp.dump(stdout,"??? ");
    puts("+++ readBer() finished OK!");
  }

  printf("Test %d: Read tag sequence\n",++tests);
  wtBuffer<unsigned char> buf((const unsigned char *)stag,strlen(stag));
  buf.append((const unsigned char *)ctag,strlen(ctag));
  BerTree ber;
  res = ber.replace(buf.readPtr(),buf.byte_size());
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: BerTree::replace() failed 0x%.4x\n",(int)res);
  } else {
    ber.root();
    ber.dump(stdout,"??? ");
    printf("??? Garbage pointer: %p\n",ber.trailer());
    puts("+++ BerTree::replace() finished OK!");
  }

  printf("Test %d: Replace first as constructed and add child\n",++tests);
  BerTag *ct = ber.root();  
  do {
    res = ct->tag(0x12,BerContentTag::BER_CONSTRUCTED,BerContentTag::BER_PRIVATE);
    if(ERR_NO_ERROR != res){
      printf("*** Error: tag() failed 0x%.4x\n",(int)res);
      break;
    }
    
    BerTag *nt = new BerTag();
    res = nt->readBer((const unsigned char *)stag,strlen(stag));
    if(ERR_NO_ERROR != res){
      printf("*** Error: readBer() failed 0x%.4x\n",(int)res);
      break;
    }
    
    ber.insertChild(nt);
  } while(0);

  if(ERR_NO_ERROR != res){
    errors++;
  } else {
    ber.root();
    ber.dump(stdout,"??? ");
    printf("??? Total Length: %d\n",ber.fullSize());
    printf("??? Garbage pointer: %p\n",ber.trailer());
    puts("+++ BerTree::insertChild() finished OK!");
  }

  FileDump stream(stdout);
  HexDump  xdump(&stream);

  printf("Test %d: Write Methods\n",++tests);  
  ber.root();
  res = ber.write(xdump);
  xdump.lineFeed();
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: write() failed 0x%.4x\n",(int)res);
  } else {
    puts("+++ BerTree::write() finished OK!");
  }

  printf("Test %d: Write through Buffer\n",++tests);  
  BufferDump bStream(128);
  ber.root();
  res = ber.write(bStream);
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: write() failed 0x%.4x\n",(int)res);
  } else {
    size_t tlen = 0;
    xdump.textf(&tlen, "??? ");
    xdump.write(bStream.get());
    xdump.lineFeed();
    puts("+++ BerTree::write() finished OK!");
  }
  
  // here we go, bStream.get() contains the serialised BER data
  printf("Test %d: Read from Buffer\n",++tests);  
  res = ber.replace((const unsigned char *)(bStream.get().readPtr()), 
		    bStream.get().byte_size());
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: replace() failed 0x%.4x\n",(int)res);
  } else {
    ber.root();
    ber.dump(stdout,"??? ");
    printf("??? Total Length: %d\n",ber.fullSize());
    printf("??? Garbage pointer: %p\n",ber.trailer());
    puts("+++ BerTree::replace() finished OK!");
  }

  printf("Test %d: Find a Tag\n",++tests);  
  ber.root();
  ct = ber.find((const unsigned char *)"\x0e",1,&res);
  if(!ct || (res != ERR_NO_ERROR)){
    errors++;
    printf("*** Error: find() failed 0x%.4x\n",(int)res);
  } else {
    ct->dump(stdout,"??? ");
    puts("+++ BerTree::find() finished OK!");
  }

  printf("Test %d: Replace a tag by assignment\n",++tests);  
  BerTag nt;
  do{
    res = nt.tag(0xabcd,BerContentTag::BER_PRIMITIVE,BerContentTag::BER_PRIVATE);
    if(res != ERR_NO_ERROR) break;
    const char *str = "Lars";
    res = nt.content((const unsigned char *)str,strlen(str));
    if(res != ERR_NO_ERROR) break;
    nt.dump(stdout,"--- ");
    *ct = nt;
    ct->dump(stdout,"??? ");
  } while(0);
  if(res == ERR_NO_ERROR){
    ber.root();
    ber.dump(stdout,"??? ");
    printf("??? Total Length: %d\n",ber.fullSize());
    printf("??? Garbage pointer: %p\n",ber.trailer());
    puts("+++ BerTag::operator=() finished OK!");
  } else {
    errors++;
    printf("*** Error: operator=() failed 0x%.4x\n",(int)res);
  }

  printf("Test %d: BerTree Deep Copy\n",++tests);  
  BerTree cBer;
  ber.root();
  ber.iterate();
  res = cBer.clone(ber);
  if((res != ERR_NO_ERROR) ||
     !(*(cBer.current()) == *(ber.current()))){
    errors++;
    cBer.root();
    cBer.dump(stdout,"??? ");
    printf("*** Error: clone() failed 0x%.4x\n",(int)res);
  } else {
    cBer.root();
    cBer.dump(stdout,"??? ");
    printf("??? Total Length: %d\n",cBer.fullSize());
    printf("??? Garbage pointer: %p\n",cBer.trailer());
    puts("+++ BerTag::clone() finished OK!");
  }

  printf("Test %d: BerContentTag::TagString -- metacode \n",++tests);  
  typedef BerContentTag::TagString<0x451b,BerContentTag::BER_CONSTRUCTED,BerContentTag::BER_APPLICATION> MyNewTag;

  unsigned char tmp[MyNewTag::SIZE];
  MyNewTag::write(tmp);
  for(size_t i = 0;i < MyNewTag::SIZE;i++){
    printf("??? Tagstring %u of 0x%x is 0x%.2x\n",i,MyNewTag::VALUE,tmp[i]);
  }

  if(memcmp(tmp,"\x7f\x81\x8a\x1b",4) || (MyNewTag::SIZE != 4)){
    errors++;
    puts("*** Error: incorrect TagString");
  } else {
    puts("*** BerContentTag::TagString finished OK!");
  }

  printf("Test %d: BerContentTag::TagArray -- metacode \n",++tests);    
  typedef BerContentTag::TagArray<0x451b,BerContentTag::BER_CONSTRUCTED,BerContentTag::BER_APPLICATION>::RESULT TestTagArray;
  // this produces a lazy initialisation constant, 
  // i.e. still no array in the text segment
  static const TestTagArray TagArr;
  const unsigned char *ta = TagArr();
  for(size_t i = 0;i < TestTagArray::SIZE;i++){
    printf("??? Tagstring %u of 0x%x is 0x%.2x (0x%.2x)\n",
	   i,tmp[i],TagArr[i],ta[i]);
  }

  printf("Test %d: BerTag::short init array\n",++tests);    
  typedef BerContentTag::TagString<0x451b,BerContentTag::BER_PRIMITIVE,BerContentTag::BER_APPLICATION> MyNewerTag;
  typedef BerContentTag::TagArray<0x451b,BerContentTag::BER_PRIMITIVE,BerContentTag::BER_APPLICATION>::RESULT MyNewerArray;
  unsigned char ntmp[MyNewerTag::SIZE];
  MyNewerTag::write(ntmp);
  static const MyNewerArray NTagArr;
  for(size_t i = 0;i < MyNewerTag::SIZE;i++){
    printf("??? Tagstring %u of 0x%x is 0x%.2x (0x%.2x%.2x%.2x%.2x) 0x%.2x\n",
	   i,MyNewerTag::VALUE,ntmp[i],
	   BerContentTag::TagByte<0x451b,0,BerContentTag::BER_PRIMITIVE,BerContentTag::BER_APPLICATION>::VALUE,
	   BerContentTag::TagByte<0x451b,1,BerContentTag::BER_PRIMITIVE,BerContentTag::BER_APPLICATION>::VALUE,
	   BerContentTag::TagByte<0x451b,2,BerContentTag::BER_PRIMITIVE,BerContentTag::BER_APPLICATION>::VALUE,
	   BerContentTag::TagByte<0x451b,3,BerContentTag::BER_PRIMITIVE,BerContentTag::BER_APPLICATION>::VALUE, NTagArr[i]);
  }
  BerTag NTag(ntmp,sizeof(ntmp));
  NTag.dump(stdout,"???");

  printf("\n%d tests completed with %d errors.\n",tests,errors);
  printf("Used version: %s\n",temp.VersionTag());

  return 0;  
}


#endif // TEST
