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
 * $Id: TaggedDataFile.cpp,v 1.6 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *
 * TaggedDataFile - class for organisation of data into scopes
 *
 * This defines the values:
 *
 */

#include "TaggedDataFile.h"
#include "TaggedDataFile.tag"

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
FileDump dbgStream(DEBUG_LOG);
HexDump dbgHex(&dbgStream);
#endif

const char * TaggedDataFile::VersionTag(void) const {
  return _VERSION_;
}

m_error_t TaggedDataFile::openScope(const BerContentTag& t){
  if(t.Type() != BerContentTag::BER_CONSTRUCTED) return ERR_PARAM_OPT;
  BerTag *nt = new BerTag(t);
  if(!nt) return ERR_PARAM_NULL;
#if DEBUG_CHECK(DUMP)
  nt->dump();
#endif  
  ber.appendNext(nt,true);
  newScope = true;
  return ERR_NO_ERROR;
}

const BerContentTag* TaggedDataFile::closeScope(){
  if(ber.isEmpty()) return NULL;
  if(newScope){
    return &(ber.current()->tag());
  }
  BerTag *p = ber.parent();
  if(!p) return NULL;
  return &(p->tag());
}

m_error_t TaggedDataFile::addTag(BerTag *t){  
  if(newScope){
    ber.insertChild(t,true);
    newScope = false;
    return ERR_NO_ERROR;
  }
  ber.appendNext(t,true);
  return ERR_NO_ERROR;
}

m_error_t TaggedDataFile::addTag(const BerTree& tr){  
  BerTree ctr;
  m_error_t res = ctr.clone(tr);
  if(res != ERR_NO_ERROR) return res;
  ctr.iteratorOnly(true);
  return addTag(ctr.root());
}
  
m_error_t TaggedDataFile::write(StreamDump& s) {  
  ber.root();
  return ber.write(s,true);
}

m_error_t TaggedDataFile::enterScope(const BerContentTag& t, size_t offset, bool absolute){
  newScope = false;
  BerTag *c = (absolute)? ber.firstSibling() : ber.current();
  if(!c) return ERR_PARAM_NULL;
  if(offset < 1) offset = 1;
  while(c){
    if(c->tag() == t){
      if(!(--offset)){
	if(c->tag().Type() == BerContentTag::BER_CONSTRUCTED){
	  c = ber.child();
	  if(!c) return ERR_CANCEL;
	  return ERR_NO_ERROR;
	}
	return ERR_CANCEL;
      }
    }
    c = ber.next();
  }
  return ERR_PARAM_END;
}

BerTree *TaggedDataFile::getScope(void){
  BerTag *c = ber.firstSibling();
  if(!c) return NULL;
  BerTree *t = new BerTree(c);
  if(t) t->iteratorOnly(true);
  return t;
}

// Item interface

m_error_t TaggedDataFile::addItem(const TDFItem& it){
  m_error_t res;
  BerTree *itr = it.writeTag(&res);
  if(res != ERR_NO_ERROR) return res;
  if(!itr) return ERR_PARAM_NULL;
  
  // transfer node ownership to this
  itr->iteratorOnly(true);
  res = addTag(itr->root());
  
  delete itr;
  return res;
}


m_error_t TaggedDataFile::readItem(TDFItem& it, size_t offset, bool absolute){
  m_error_t res = enterScope(it.tag(),offset,absolute);
  if((res != ERR_NO_ERROR) &&
     (res != ERR_CANCEL)) return res;
  // enter scope positions to the child, if exists
  if(res == ERR_NO_ERROR) ber.parent();

  res = it.readTag(*(ber.current()));

  // avoid rereading the same item, if absolute = false is set
  if(res == ERR_NO_ERROR) 
    if(!(ber.next())) return ERR_CANCEL;

  return res;
}

/*
 * TDFItem helpers
 *
 */

BerTree *TDFItem::initWrite(BerTag *& tg, m_error_t *&err) const {
  if(err) *err = ERR_MEM_AVAIL;
  tg = new BerTag(Tag);
  if(!tg) return NULL;
  
  BerTree *tr = new BerTree(tg);
  if(!tr){
    delete tg;
    tg = NULL;
    return NULL;
  }
  // destroy nodes with destruction of BerTree object
  tr->iteratorOnly(false);
  if(err) *err = ERR_NO_ERROR;
  
  return tr;
}

void TDFItem::finishWrite(BerTree*& tr, BerTag*& tg, m_error_t*& err, m_error_t& ierr) const {
  if(tg){
    delete tg;
    tg = NULL;
  }
  if(ierr != ERR_NO_ERROR){
    delete tr;
    tr = NULL;
  }
  if(err) *err = ierr;
}

/*
 * The TDFDataHeader Class
 *
 */

// Just to have the VTable somewhere
TDFItem::~TDFItem() {}

m_error_t TDFDataHeader::init(const char *n, const char *auth) {
  Date = time(NULL);
  Name = (n)? strdup(n) : NULL;
  Author = (auth)? strdup(auth) : NULL;
  if((n && !Name) || (auth && !Author)){
    if(Name) ::free(const_cast<char *>(Name));
    if(Author) ::free(const_cast<char *>(Author));
    return ERR_MEM_AVAIL;
  }
  return ERR_NO_ERROR;
}

m_error_t TDFDataHeader::name(const char *n){
  if(Name) ::free(const_cast<char *>(Name));
  if(n){
    Name = strdup(n);
    if(!Name) return ERR_MEM_AVAIL;
  } else Name = NULL;
  return ERR_NO_ERROR;
}

m_error_t TDFDataHeader::author(const char *n){
  if(Author) ::free(const_cast<char *>(Author));
  if(n){
    Author = strdup(n);
    if(!Author) return ERR_MEM_AVAIL;
  } else Author = NULL;
  return ERR_NO_ERROR;
}

BerTree *TDFDataHeader::writeTag(m_error_t *err) const{
  if(err) *err = ERR_MEM_AVAIL;
  BerTag *tg = new BerTag((int)TaggedDataFile::HEADER,
			   BerContentTag::BER_CONSTRUCTED, 
			   BerContentTag::BER_APPLICATION);
  if(!tg) return NULL;

  BerTree *tr = new BerTree(tg);
  if(!tr){
    delete tg;
    return NULL;
  }
  // delete node upon deletion of BerTree
  tr->iteratorOnly(false);

  m_error_t ierr = ERR_MEM_AVAIL;
  do {
    tg = asn.writeIntPacked(Type);
    if(!tg) break;
    tr->insertChild(tg,true);
    tg = asn.writeUTC(Date, &ierr);
    if(!tg || (ierr != ERR_NO_ERROR)) break;    
    tr->insertNext(tg,true);
    tg = asn.writePrintableString(Name, &ierr);
    if(!tg || (ierr != ERR_NO_ERROR)) break;    
    tr->insertNext(tg,true);
    tg = asn.writePrintableString(Author, &ierr);
    if(!tg || (ierr != ERR_NO_ERROR)) break;    
    tr->insertNext(tg,true);
    tg = NULL;
  } while(0);
  if(tg){
    delete tg;
    tg = NULL;
  }
  if(ierr != ERR_NO_ERROR){
    delete tr;
    tr = NULL;
  }

  if(err) *err = ierr;
  return tr;
}

m_error_t TDFDataHeader::readTag(const BerTag& t){
  if(!TaggedDataFile::tHeader::isEqual(t.tag().readPtr())) 
    return ERR_PARAM_TYP;
  BerTree tr(const_cast<BerTag *>(&t));
  // this justifies the const_cast<>
  tr.iteratorOnly(true);
  BerTag *tg = tr.child();
  if(!tg) return ERR_PARAM_UDEF;
  m_error_t err = asn.readInt(*tg,Type);
  if(err != ERR_NO_ERROR){
    Type = 1;
    return err;
  }
  tg = tr.next();
  if(!tg) return ERR_PARAM_UDEF;
  struct tm tmp_tm;
  err = asn.readUTC(*tg,&tmp_tm);
  if(err != ERR_NO_ERROR) return err;
  Date = mktime(&tmp_tm);
  tg = tr.next();
  if(!tg) return ERR_PARAM_UDEF;
  if(Name) ::free(const_cast<char *>(Name));
  size_t bs = tg->c_size()+1;
  Name = (const char *)malloc(bs);
  // const_cast<> to initialise, it's constant all time thereafter
  err = asn.readPrintableString(*tg,const_cast<char *>(Name),bs);
  if(err != ERR_NO_ERROR){
    ::free(const_cast<char *>(Name));
    Name = NULL;
    return err;
  }
  tg = tr.next();
  if(!tg) return ERR_PARAM_UDEF;
  if(Author) ::free(const_cast<char *>(Author));
  bs = tg->c_size()+1;
  Author = (const char *)malloc(bs);
  // const_cast<> to initialise, it's constant all time thereafter
  err = asn.readPrintableString(*tg,const_cast<char *>(Author),bs);
  if(err != ERR_NO_ERROR){
    ::free(const_cast<char *>(Author));
    Author = NULL;
    return err;
  }

  return ERR_NO_ERROR;
}

m_error_t TDFDataHeader::dump(FILE *f, const char *prefix) const {
  if(!f) return ERR_PARAM_NULL;
  if(!prefix) prefix = "";
  if(0 > fprintf(f,"%s   Type: %d\n",prefix,Type)) 
    return ERR_FILE_WRITE;
  if(0 > fprintf(f,"%s   Date: %s",prefix,ctime(&Date))) 
    return ERR_FILE_WRITE;
  if(0 > fprintf(f,"%s   Name: %s\n",prefix,(Name)? Name : "(empty)")) 
    return ERR_FILE_WRITE;
  if(0 > fprintf(f,"%s Author: %s\n",prefix,(Author)? Author : "(empty)")) 
    return ERR_FILE_WRITE;

  return ERR_NO_ERROR;
}

const char * TDFDataHeader::VersionTag(void) const {
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
#include <HexDump.h>
#include <wtBufferDump.h>

int main(int argc, char *argv[]){
  m_error_t res;
  size_t tests, errors;

  tests = errors = 0;

  try{
    FileDump stream(stdout);
    HexDump  xdump(&stream);

    printf("Test %d: CTOR\n",++tests);
    TaggedDataFile tdf;
    puts("+++ TaggedDataFile::CTOR() finished OK!");
    
    printf("Test %d: reset()\n",++tests);
    tdf.reset();
    puts("+++ TaggedDataFile::reset() finished OK!");
    
    printf("Test %d: openScope()\n",++tests);
    BerContentTag tag;
    res = tag.replace((int)ASN1IO::ASN1_SEQ,
		      BerContentTag::BER_CONSTRUCTED, 
		      BerContentTag::BER_UNIVERSAL);
    if(res != ERR_NO_ERROR){
      printf("*** Error: Tag::replace() failed 0x%.4x\n",(int)res);
      return 0;
    }
    printf("??? Tag::replace() succeeded\n");
    res = tdf.openScope(tag);
    if(res != ERR_NO_ERROR){
      errors++;
      printf("*** Error:openScope() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ TaggedDataFile::openScope() finished OK!");
    }
    
    printf("Test %d: closeScope()\n",++tests);
    const BerContentTag *rtag = tdf.closeScope();
    if(!rtag || !(*rtag == tag)){
      errors++;
      if(!rtag){
	printf("*** Error:closeScope() returned NULL\n");
      } else {
	printf("*** Error:closeScope() returned wrong Tag: ");
	tag.write(xdump);
      }
    } else {
      puts("+++ TaggedDataFile::closeScope() finished OK!");
    }

    printf("Test %d: openScope() (2nd)\n",++tests);
    res = tdf.openScope(tag);
    if(res != ERR_NO_ERROR){
      errors++;
      printf("*** Error:openScope() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ TaggedDataFile::openScope() finished OK!");
    }

    printf("Test %d: addTag(*)\n",++tests);
    BerTag *tia = new BerTag((int)(ASN1IO::ASN1_PRINTABLE_STRING),
			     BerContentTag::BER_PRIMITIVE, 
			     BerContentTag::BER_UNIVERSAL);
    if(!tia){
      puts("*** Error: could not allocate BerTag!");
      return 0;
    }
    const char *tia_txt = "This is a test text!";
    res = tia->content((const unsigned char *)tia_txt,strlen(tia_txt));      
    // detach not necessary, because tia_txt is valid for the same scope as
    // tdf is valid
    if(res != ERR_NO_ERROR){
      errors++;
      printf("*** Error: BerTag::content() failed 0x%.4x\n",(int)res);
    } else {
      res = tdf.addTag(tia);
      if(res != ERR_NO_ERROR){
	errors++;
	printf("*** Error: addTag(*) failed 0x%.4x\n",(int)res);
      } else {
	puts("+++ TaggedDataFile::addTag(*) finished OK!");
      }
    }
    
    printf("Test %d: closeScope() (2nd)\n",++tests);
    rtag = tdf.closeScope();
    if(!rtag || !(*rtag == tag)){
      errors++;
      if(!rtag){
	printf("*** Error:closeScope() returned NULL\n");
      } else {
	printf("*** Error:closeScope() returned wrong Tag: ");
	tag.write(xdump);
      }
    } else {
      puts("+++ TaggedDataFile::closeScope() finished OK!");
    }
    
    printf("Test %d: write()\n",++tests);
    res = tdf.write(xdump);
    xdump.lineFeed();
    if(res != ERR_NO_ERROR){
      errors++;
      printf("*** Error: write() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ TaggedDataFile::write() finished OK!");
    }

    BufferDump bFile;

    printf("Test %d: write() to buffer for re-import\n",++tests);
    res = tdf.write(bFile);
    if(res != ERR_NO_ERROR){
      errors++;
      printf("*** Error: write() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ TaggedDataFile::write() finished OK!");
    }

    printf("Test %d: reset()\n",++tests);
    tdf.reset();
    puts("+++ TaggedDataFile::reset() finished OK!");

    printf("Test %d: read()\n",++tests);    
    res = tdf.read(bFile.get());
    if(res != ERR_NO_ERROR){
      errors++;
      printf("Test %d: write() to buffer for re-import\n",++tests);
    } else {
      puts("+++ TaggedDataFile::read() finished OK!");
    }

    printf("Test %d: rewind()\n",++tests);
    tdf.rewind();
    puts("+++ TaggedDataFile::rewind() finished OK!");
    
    printf("Test %d: enterScope() for empty scope\n",++tests);
    res = tdf.enterScope(tag);
    if(res != ERR_CANCEL){
      errors++;
      printf("*** Error: enterScope() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ TaggedDataFile::enterScope() finished OK!");
    }

    printf("Test %d: enterScope() for non-empty scope\n",++tests);
    res = tdf.enterScope(tag,2);
    if(res != ERR_NO_ERROR){
      errors++;
      printf("*** Error: enterScope() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ TaggedDataFile::enterScope() finished OK!");
    }

    printf("Test %d: getScope() for non-empty scope\n",++tests);
    BerTree *scope = tdf.getScope();
    if(!scope){
      errors++;
      printf("*** Error: getScope() failed\n");
    } else {
      scope->dump(stdout,"??? ");
      puts("+++ TaggedDataFile::getScope() finished OK!");
    }

    printf("Test %d: rewind()\n",++tests);
    tdf.rewind();
    puts("+++ TaggedDataFile::rewind() finished OK!");

    printf("Test %d: TDFDataHeader()\n",++tests);
    TDFDataHeader tdf_head("Test Header","Dr. Lars Hanke");
    res = tdf_head.dump(stdout,"??? ");
    if(res != ERR_NO_ERROR){
      errors++;
      printf("*** Error: TDFDataHeader::dump() failed 0x%.4x\n",(int)res);
    } else {	     
      puts("+++ TDFDataHeader::dump() and CTOR finished OK!");
    }

    printf("Test %d: TDFDataHeader::writeTag()\n",++tests);
    scope = tdf_head.writeTag(&res);
    if(scope){
      scope->root();
      scope->dump(stdout,"??? ");      
    }
    if(!scope || (res != ERR_NO_ERROR)){
      printf("*** Error: TDFDataHeader::writeTag() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ TDFDataHeader::writeTag() finished OK!");
    }

    printf("Test %d: TDFDataHeader::readTag()\n",++tests);
    tia = scope->root();
    TDFDataHeader *thd = new TDFDataHeader(*tia);
    if(thd){
      thd->dump(stdout,"??? ");
      puts("+++ TDFDataHeader::readTag() finished OK!");
    } else {
      errors++;
      puts("*** Error: TDFDataHeader::readTag() returned NULL!");
    }

    printf("Test %d: TaggedDataFile::addItem()\n",++tests);
    tdf.rewind();
    res = tdf.addItem(tdf_head);
    if(res != ERR_NO_ERROR){
      errors++;
      printf("*** Error: TaggedDataFile::addItem() failed 0x%.4x\n",(int)res);
    } else {
      printf("??? ");
      tdf.write(xdump);
      xdump.lineFeed();
      puts("+++ TaggedDataFile::addItem() finished OK!");
    }

    printf("Test %d: TaggedDataFile::readItem()\n",++tests);
    delete thd;
    thd = new TDFDataHeader();
    res = tdf.readItem(*thd);
    if(res != ERR_CANCEL){
      errors++;
      printf("*** Error: TaggedDataFile::readItem() failed 0x%.4x\n",(int)res);
    } else {
      thd->dump(stdout,"??? ");
      puts("+++ TaggedDataFile::readItem() finished OK!");
    }

    printf("\n%d tests completed with %d errors.\n",tests,errors);
    printf("Used version: %s\n",tdf.VersionTag());

    return 0;  
  }
  // m_error_t catched as int
  catch(int err){
    printf("*** Exception thrown: 0x%.4x\n",err);
  }
}

#endif // TEST
