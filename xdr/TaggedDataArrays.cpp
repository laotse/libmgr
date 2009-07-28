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
 * $Id: TaggedDataArrays.cpp,v 1.6 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *
 * TaggedDataFile - class for organisation of data into scopes
 *
 * This defines the values:
 *
 */

#define DEBUG_MARK 1
#define DEBUG_DUMP 2

#define DEBUG (DEBUG_MARK | DEBUG_DUMP)
#ifdef DEBUG
# include <stdio.h>
# include <HexDump.h>
#endif
#include <mgrDebug.h>

#include "TaggedDataArrays.h"
#include "TaggedDataArrays.tag"

using namespace mgr;

#ifdef DEBUG
FileDump dbgStream(DEBUG_LOG);
HexDump dbgHex(&dbgStream);
#endif

const unsigned char _TDArray::bIntArray[tIntArray::SIZE] = {
  BerContentTag::TagByte<(int)TaggedDataFile::INT_ARRAY, 0,
  BerContentTag::BER_CONSTRUCTED, BerContentTag::BER_APPLICATION >::VALUE
};

const unsigned char _TDArray::bDoubleArray[tDoubleArray::SIZE] = {
  BerContentTag::TagByte<(int)TaggedDataFile::DOUBLE_ARRAY, 0,
  BerContentTag::BER_CONSTRUCTED, BerContentTag::BER_APPLICATION >::VALUE
};

BerTree *_TDArray::arrayCoreWrite(const BerContentTag& mtag, 
				  const _wtBuffer& c,  
				  m_error_t& err) const {
  BerTag *tia = new BerTag(mtag);
  if(!tia){ 
    err = ERR_MEM_AVAIL;
    return NULL;
  }
  BerTree *st = new BerTree(tia);	
  // make st free all nodes in destructor
  st->iteratorOnly(false);
  st->root();
  err = ERR_NO_ERROR;
  do {
    if( c.rec_size() <= 0 ) {
      err = ERR_PARAM_TYP;
      break;
    }
    if( c.byte_size() % c.rec_size() ){
      err = ERR_PARAM_LEN;
      break;
    }
    tia = asn.writeIntPacked<size_t>(c.byte_size() / c.rec_size());    
    if(!tia){
      err = ERR_MEM_AVAIL;
      break;
    }
    st->insertChild(tia);
    tia = new BerTag((int)ASN1IO::ASN1_SEQ,
		     BerContentTag::BER_PRIMITIVE, 
		     BerContentTag::BER_UNIVERSAL);
    if(!tia){
      err = ERR_MEM_AVAIL;
      break;
    }
    err = tia->content(c);
    st->appendChild(tia,true);
    tia = NULL;
  } while(0);
  if(err != ERR_NO_ERROR){
    if(tia) delete tia;
    delete st;
    st = NULL;
  }
    
  return st;
}

m_error_t _TDArray::arrayCoreRead(const BerContentTag& mtag,
				  const BerTag *arry, 
				  _wtBuffer& c){
  if(!arry) return  ERR_PARAM_NULL;
  if(arry->tag() != mtag) return ERR_PARAM_OPT;

  // since BerTree is only iterator, const_cast<> is okay
  BerTree at(const_cast<BerTag *>(arry));
  at.iteratorOnly(true);

  arry = at.child();
  if(!arry){
    // empty array
    c.free();
    return ERR_NO_ERROR;
  }

  size_t num;
  m_error_t res = asn.readInt<size_t>(*arry,num);
  if(res != ERR_NO_ERROR) return res;

  arry = at.next();
  if(!arry) return ERR_PARAM_UDEF;

  size_t rec = arry->c_size() / num;
  if(arry->c_size() % num) return ERR_PARAM_LEN;

  if(rec != (size_t)(c.rec_size())){
    pdbg("*** Record size mismatch: c_size: %u, num: %u, rec_size: %d\n",
	 arry->c_size(), num, c.rec_size());
    return ERR_PARAM_TYP;
  }
  
  // c is a _wtBuffer here, i.e. size in replace is bytes not records
  return c.replace(arry->content().readPtr(), num * rec);
}

const char * _TDArray::VersionTag(void) const {
  return _VERSION_;
}


BerTree *_TDArray::writeTagCore(const BerContentTag& mtag, 
				const _wtBuffer& A, m_error_t *err) const {
  m_error_t xerr = ERR_NO_ERROR;
  m_error_t& res = (err)? *err: xerr;
  BerTree *st = arrayCoreWrite(mtag, A, res);
  if(!st || (res != ERR_NO_ERROR)){
    if(res == ERR_NO_ERROR) res = ERR_INT_STATE;
    if(st) delete st;
    st = NULL;
  }
  return st;
}

/*
 * Integer Implementation
 *
 */

namespace mgr {

template<>
m_error_t TDIntArray::dump(FILE *f, const char *prefix) const {
  if(!f) return ERR_PARAM_NULL;
  if(!A.size()){
    if(0 > fprintf(f,"%s(empty)\n",prefix)) return ERR_FILE_WRITE;
    return ERR_NO_ERROR;
  }
  const XDR::Int *d = A.readPtr();
  for(size_t i=0;i<A.size();i++,d++){
    int v = XDR::xdrRead<XDR::Int>(d);
    if(0 > fprintf(f,"%s%d: %d\n",prefix,i,v)) return ERR_FILE_WRITE;
  }
  return ERR_NO_ERROR;
}

/*
 * Short Implemetation
 *
 */

template<>
m_error_t TDShortArray::dump(FILE *f, const char *prefix) const {
  if(!f) return ERR_PARAM_NULL;
  if(!A.size()){
    if(0 > fprintf(f,"%s(empty)\n",prefix)) return ERR_FILE_WRITE;
    return ERR_NO_ERROR;
  }
  const XDR::Short *d = A.readPtr();
  for(size_t i=0;i<A.size();i++,d++){
    int v = XDR::xdrRead<XDR::Short>(d);
    if(0 > fprintf(f,"%s%d: %d\n",prefix,i,v)) return ERR_FILE_WRITE;
  }
  return ERR_NO_ERROR;
}


/*
 * Double Implementation
 *
 */

template<>
m_error_t TDDoubleArray::dump(FILE *f, const char *prefix) const {
  if(!f) return ERR_PARAM_NULL;
  if(!A.size()){
    if(0 > fprintf(f,"%s(empty)\n",prefix)) return ERR_FILE_WRITE;
    return ERR_NO_ERROR;
  }
  const XDR::Double *d = A.readPtr();
  for(size_t i=0;i<A.size();i++,d++){
    double v = XDR::xdrRead<XDR::Double>(d);
    if(0 > fprintf(f,"%s%d: %lf\n",prefix,i,v)) return ERR_FILE_WRITE;
  }
  return ERR_NO_ERROR;
}

} // namespace mgr for template specilization

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
    const int arri[3] = {1,2,3};
    TDIntArray iarr(arri,3);
    puts("+++ TDIntArray::CTOR() finished OK!");

    printf("Test %d: TDIntArray::dump()\n",++tests);
    res = iarr.dump(stdout,"??? ");
    if(res != ERR_NO_ERROR){
      errors++;
      printf("*** Error: TDIntArray::dump() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ TDIntArray::dump() finished OK!");
    }

    printf("Test %d: TDIntArray::writeTag()\n",++tests);
    BerTree *tr = iarr.writeTag(&res);
    if(tr){
      tr->root();
      tr->dump(stdout,"???");
    }
    if(!tr || (res != ERR_NO_ERROR)){
      errors++;
      printf("*** Error: TDIntArray::writeTag() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ TDIntArray::writeTag() finished OK!");
    }

    printf("Test %d: TDIntArray::readTag()\n",++tests);
    const int arri2[3] = {10,20,30};
    TDIntArray iarr2(arri2,3);
    iarr2.dump(stdout,"### ");
    res = iarr2.readTag(tr->root());
    iarr2.dump(stdout,"??? ");
    if(res != ERR_NO_ERROR){
      errors++;
      printf("*** Error: TDIntArray::readTag() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ TDIntArray::readTag() finished OK!");
    }

    printf("Test %d: TDIntArray::writeTag() delete BerTree\n",++tests);
    tr->iteratorOnly(false);
    if(tr) delete tr;
    puts("+++ TDIntArray::writeTag() delete BerTree finished OK!");
    tr = NULL;

    printf("Test %d: CTOR\n",++tests);
    const double arrd[3] = {1.0,2.25,3.33};
    TDDoubleArray darr(arrd,3);
    puts("+++ TDDoubleArray::CTOR() finished OK!");

    printf("Test %d: TDDoubleArray::dump()\n",++tests);
    res = darr.dump(stdout,"??? ");
    if(res != ERR_NO_ERROR){
      errors++;
      printf("*** Error: TDDoubleArray::dump() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ TDDoubleArray::dump() finished OK!");
    }

    printf("Test %d: TDDoubleArray::writeTag()\n",++tests);
    tr = darr.writeTag(&res);
    if(tr){
      tr->root();
      tr->dump(stdout,"???");
    }
    if(!tr || (res != ERR_NO_ERROR)){
      errors++;
      printf("*** Error: TDDoubleArray::writeTag() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ TDDoubleArray::writeTag() finished OK!");
    }

    printf("Test %d: TDDoubleArray::readTag()\n",++tests);
    res = darr.readTag(tr->root());
    darr.dump(stdout,"??? ");
    if(res != ERR_NO_ERROR){
      errors++;
      printf("*** Error: TDDoubleArray::readTag() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ TDDoubleArray::readTag() finished OK!");
    }

        
    printf("\n%d tests completed with %d errors.\n",tests,errors);
    printf("Used version: %s\n",iarr.VersionTag());

    return 0;  
  }
  // m_error_t catched as int
  catch(int err){
    printf("*** Exception thrown: 0x%.4x\n",err);
    return 0;
  }
}

#endif // TEST
