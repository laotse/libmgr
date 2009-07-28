/*
 *
 * Extensible write-through buffer
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: wtBuffer.cpp,v 1.6 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  wtBuffer  - an extensible auto-allocation buffer
 *              with write-through capability
 *
 * This defines the values:
 *  DEFAULT_WTBUFFER_CHUNK
 *
 */

/*! \file wtBuffer.cpp
    \brief Extensible reference-counted buffer

    \author Dr. Lars Hanke
    \date 2006    
*/


//#define DEBUG
//! Debug allocation and deallocation - very noisy!
#define DEBUG_ALLOCATE 1
#define DEBUG (DEBUG_ALLOCATE)
#include <mgrDebug.h>

#ifdef DEBUG
# include <stdio.h>
#endif

#include "wtBuffer.h"
#include "wtBuffer.tag"

using namespace mgr;

/*! \param s Size of __wtBuffer
    \retval ERR_NO_ERROR or error code if fails

    Allocate a new __wtBuffer for variable data. If the new buffer
    cannot be claimed, _wtBuffer is initialized as empty and
    ERR_MEM_AVAIL is returned. Otherwise, __wtBuffer and _wtBuffer
    are ready to be written.

    \warning This protected function expects that no variable
    buffer is present. It does not check. This is why it's
    protected!
*/
m_error_t _wtBuffer::initVar(const size_t&s){
  buf.var = new __wtBuffer(s);
  if(!buf.var || !(buf.var->ptr())){
    if(buf.var) delete buf.var;
    initEmpty();
    return ERR_MEM_AVAIL;
  }
  buf.var->lock();
  xpdbg(ALLOCATE,"### __wtBuffer %p created!\n",buf.var);
  isFix = false;
  return ERR_NO_ERROR;
}


void _wtBuffer::init_wtBuffer(const size_t& c){
  chunk = c;
  accessWrite = false;
  initEmpty();
}

/*! \param b _wtBuffer to copy

    The copy CTOR copies the buffer metrics, i.e. record and chunk size.
    If the buffer holds a reference, the reference is copied. If the buffer
    holds an actual __wtBuffer instance, the copy obtains a lock on it.
    There is never a second instance of the actual data generated.
*/
_wtBuffer::_wtBuffer(const class _wtBuffer& b){
  init_wtBuffer(b.chunk);
  accessWrite = b.accessWrite;
  isFix = b.isFix;
  if(isFix){
    buf.fix = b.buf.fix;
  } else {
    b.buf.var->lock();
    buf.var = b.buf.var;
  }
  length = b.length;
}

/*! \param b _wtBuffer to assign from

    The assignment operator acts much like the copy constructor.
    Buffer metrics is copied, while buffer content is referenced.

    \sa _wtBuffer(const class _wtBuffer& b)
*/
class _wtBuffer& _wtBuffer::operator=(const class _wtBuffer& b){
  chunk = b.chunk;
  accessWrite = b.accessWrite;

  if(!isFix){
    if(buf.var && buf.var->release()){
      xpdbg(ALLOCATE,"### __wtBuffer %p ran out of scope\n",buf.var);
      delete buf.var;
    }
    buf.var = NULL;    
  }
  isFix = b.isFix;
  if(isFix){
    buf.fix = b.buf.fix;
  } else {
    b.buf.var->lock();
    buf.var = b.buf.var;
  }
  length = b.length;

  return *this;
}  

/*! Shrinking is performed by reducing the length property. No 
    re-allocation is performed.
    Enlarging for references is only performed, if copy is true.
    In this case a branch() is performed and data is read up to the
    original length. The remainder is undefined.
    Enlarging for __wtBuffer is either performed in place for 
    exclusively owned containers or by creating a new instance
    and copy the contents.

    \todo Check, if the branch() read() combination for fixed buffer
    enlargement does not perform double copy.
*/
m_error_t _wtBuffer::trunc(const size_t& l, bool copy){
  xpdbg(ALLOCATE,"### Truncate %p to byte size: %d\n",this,l);

  m_error_t error = ERR_NO_ERROR;
  if(!isFix){    
    // okay, we may do whatever we like
    if(l > buf.var->size()){
      size_t bl = roundChunk(l,error);
      xpdbg(ALLOCATE,"### - trunc() allocate byte size: %d\n",bl);
      if(error != ERR_NO_ERROR) return error; 
      if(buf.var->release()){
	buf.var->resize(bl);
	if(!buf.var->ptr()){
	  error = ERR_MEM_AVAIL;
	  initEmpty();
	  return error;
	}
	buf.var->lock();      
      } else {
	__wtBuffer *old = buf.var;
	error = initVar(bl);
	if(error != ERR_NO_ERROR) return error;
	*buf.var << *old;
      }
    }
    length = l;
    xpdbg(ALLOCATE,"### - trunc() Length %d bytes of %d at %p\n",
	  length,buf.var->size(),buf.var->ptr());

    return ERR_NO_ERROR;
  }

  // we have a fixed buffer, shrink only
  // or extend, if copy allowed
  if(l > length){
    // we must not copy, we're lost
    if(!copy && buf.fix) return ERR_PARAM_LEN;
    size_t bl = roundChunk(l, error);
    if(error != ERR_NO_ERROR) return error;      
    const void *old = buf.fix;
    xpdbg(ALLOCATE,"### malloc() bytes: %u\n",bl);
    error = branch(bl);
    if(error != ERR_NO_ERROR) return error;
    buf.var->read(old,length);
  } 
  length = l;

  return ERR_NO_ERROR;
}

/*! \param l size to allocate in octets
    \retval Error code as defined in mgrError.h

    Discards current contents and instantiates a new __wtBuffer
    using the size specified.

    \warning No size adaption for records or chunks is done. This
    is a legacy function. Do not use.

    \sa allocateRecs()
*/
m_error_t _wtBuffer::allocate(const size_t& l){
  xpdbg(ALLOCATE,"### Allocate %p byte size: %d\n",this,l);

  if(!isFix) free();
  m_error_t error = initVar(l);
  if(error != ERR_NO_ERROR) return error;
  length = l;
  return ERR_NO_ERROR;
}

/*! \param s size of branched buffer
    \retval error code as defined in mgrError.h

    If the buffer is not owned exclusively, a new __wtBuffer
    is instantiated and initialised from the old contents. Any
    previous __wtBuffer is relased. If s is smaller than the 
    originally allocated size, the new _wtBuffer may use less memory
    than the old one.

    \warning s is not checked for plausbility, e.g. 
    s > __wtBuffer::allocated. This is why it is protected.
*/
m_error_t _wtBuffer::branch(const size_t& s){
  const void *old = NULL;
  bool mustBranch = true;
  if(isFix){
    // mustBranch is true here
    old = buf.fix;
  } else if(buf.var) {
    old = buf.var->ptr();
    mustBranch = buf.var->isShared();
  } 
  if(mustBranch){
    xpdbg(ALLOCATE,"### Branch %s buffer %p length %u into space %u\n",(isFix)?"fixed":"allocated",this,length,s);
    if(isFix) buf.var = new __wtBuffer(s,buf.fix);
    else {
      buf.var->release();
      buf.var = new __wtBuffer(s,buf.var->ptr());
    }
    if(!buf.var || (s && !buf.var->ptr())){
      if(buf.var) delete buf.var;
      initEmpty();
      return ERR_MEM_AVAIL;
    }    
    buf.var->lock();
    xpdbg(ALLOCATE,"### __wtBuffer %p created!\n",buf.var);
    isFix = false;
  }
  return ERR_NO_ERROR;
}

/*! \param s new length in octets
    \retval size actually usable

    This function enlarges or shrinks the usable length of a _wtBuffer
    without any reallocation. While shrinking is always possible, enlarging
    is only possible up to the size allocated within __wtBuffer container.
    A fixed reference cannot be enlarged.

    The function returns the number of octets granted. This may be less
    than requested.
*/
size_t _wtBuffer::accept(const size_t& s){
  if(s<=length){
    length = s;
    return length;
  }
  if(isFix) return length;
  if(!buf.var) return 0;
  length = (s<buf.var->size())? s : buf.var->size();
  return length;
}


m_error_t _wtBuffer::prepend(const void *data, const size_t& l){
  size_t old = length;
  m_error_t error = trunc(length + l, true);
  if(error != ERR_NO_ERROR) return error;
  memmove(buf.var->cptr() + l, buf.var->cptr(), old);
  memcpy(buf.var->cptr(),data,l);
  return ERR_NO_ERROR;
}

m_error_t _wtBuffer::insert(const size_t& at, const size_t& consume, const void *data, const size_t& len){
  if(!data) return ERR_PARAM_NULL;
  if(at + consume > length) return ERR_PARAM_LEN;
  if(consume >= len){
    m_error_t ierr = branch();
    if(ierr != ERR_NO_ERROR) return ierr;
    memcpy(buf.var->cptr() + at,data,len);
    if(consume > len){
      memmove(buf.var->cptr() + at + len, 
	      buf.var->cptr() + at + consume, 
	      length - at - consume);
      length -= consume - len;
    }
    return ERR_NO_ERROR;
  }
  // consume < len
  size_t old = length;
  m_error_t ierr = trunc(length + len - consume, true);
  if(ierr != ERR_NO_ERROR) return ierr;
  memmove(buf.var->cptr() + at + len, 
	  buf.var->cptr() + at + consume, 
	  old - at - consume);
  memcpy(buf.var->cptr() + at, data, len);
  return ERR_NO_ERROR;
}

/*! \param b Buffer to campare with
    \retval bool, true if different

    Two buffers are considered equal, if they have the same length
    and contain the same data according to this length. They may well
    refer to different addresses. NULL pointers are always different,
    even two NULL pointers are considered different from each other.
*/
bool _wtBuffer::operator!=(const _wtBuffer& b) const {
  if(length != b.length) return true;
  if(!length) return false;
  if(!rawPtr() || !b.rawPtr()) return true;
  return memcmp(rawPtr(),b.rawPtr(),length);
}


const char * _wtBuffer::VersionTag(void) const{
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

// we need a custom class to have an implementation of
// replace(const char *)
class cwtBuffer : public wtBuffer<char> {
public:
  inline m_error_t replace(const char *s, const size_t& l){
    return wtBuffer<char>::replace(s,l);
  }
  inline m_error_t replace(const char *s){
    return replace(s,strlen(s)+1);
  }

};


int main(int argc, char *argv[]){
  const char *s;
  m_error_t res;
  size_t tests, errors;

  tests = errors = 0;

  printf("Test %d: Write string to buffer\n",++tests);
  cwtBuffer temp;
  res = temp.replace("This is a minimal buffer test");
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: replace() failed 0x%.4x\n",(int)res);
  } else {
    printf("??? %s\n",temp.readPtr());
    puts("+++ replace() finished OK!");
  }

  printf("Test %d: Append string by trunc() append()\n",++tests);
  do{
    // truncate trailing 0
    //printf("### size: %d\n",temp.size());
    res = temp.trunc(temp.size()-1);
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: trunc() failed 0x%.4x\n",(int)res);
      break;
    }
    //printf("### size: %d\n",temp.size());
    s=" Something appended!";
    res = temp.append(s,strlen(s)+1);
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: append() failed 0x%.4x\n",(int)res);
      break;
    }
    s = temp.readPtr();
    if(strlen(s)+1 != temp.size()){
      printf("*** Error: Size is %d was expected as %d\n",temp.size(),strlen(s)+1);
      errors++;
      break;
    }
    printf("??? %s\n",temp.readPtr());
    puts("+++ append() finished OK!");    
  }while(0);

  printf("Test %d: Prepend string by trunc() append()\n",++tests);
  s="This is prepended... ";
  do{
    res = temp.prepend(s,strlen(s));
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: prepend() failed 0x%.4x\n",(int)res);
      break;
    }
    s=temp.readPtr();
    if(strlen(s)+1 != temp.size()){
      printf("*** Error: Size is %d was expected as %d\n",temp.size(),strlen(s)+1);
      errors++;
      break;
    }
    printf("??? %s\n",temp.readPtr());
    puts("+++ prepend() finished OK!");    
  }while(0);
  
  temp.free();
  temp.Chunk(10);
  puts("Reduced chunk size to 10 byte and repeat!");

  printf("Test %d: Write string to buffer\n",++tests);
  res = temp.replace("This is a minimal buffer test");
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: replace() failed 0x%.4x\n",(int)res);
  } else {
    printf("??? %s\n",temp.readPtr());
    puts("+++ replace() finished OK!");
  }

  printf("Test %d: Append string by trunc() append()\n",++tests);
  do{
    // truncate trailing 0
    res = temp.trunc(temp.size()-1);
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: trunc() failed 0x%.4x\n",(int)res);
      break;
    }
    s=" Something appended!";
    res = temp.append(s,strlen(s)+1);
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: append() failed 0x%.4x\n",(int)res);
      break;
    }
    s = temp.readPtr();
    if(strlen(s)+1 != temp.size()){
      printf("*** Error: Size is %d was expected as %d\n",temp.size(),strlen(s)+1);
      errors++;
      break;
    }
    printf("??? %s\n",temp.readPtr());
    puts("+++ append() finished OK!");    
  }while(0);

  printf("Test %d: Prepend string by trunc() append()\n",++tests);
  s="This is prepended... ";
  do{
    res = temp.prepend(s,strlen(s));
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: prepend() failed 0x%.4x\n",(int)res);
      break;
    }
    s=temp.readPtr();
    if(strlen(s)+1 != temp.size()){
      printf("*** Error: Size is %d was expected as %d\n",temp.size(),strlen(s)+1);
      errors++;
      break;
    }
    printf("??? %s\n",temp.readPtr());
    puts("+++ prepend() finished OK!");    
  }while(0);
  
  printf("Test %d: allocate() large buffer\n",++tests);
  wtBuffer<unsigned char> fbuf;
  res = fbuf.allocate(4188);
  if(res != ERR_NO_ERROR){
    errors++;
    printf("*** Error: allocate() returned 0x%.4x\n",(int)res);
  } else {
    puts("+++ allocate() finished OK!");
  }

  printf("Test %d: allocation logics: replace()\n",++tests);
  const char *is = "This is fixed!";
  res = temp.replace(is);
  if((res != ERR_NO_ERROR) || (temp.readPtr() != is)){
    errors++;
    printf("*** Error: replace() returned 0x%.4x\n",(int)res);
    printf("*** Error: pointers are %p == %p\n",is,temp.readPtr());
  } else {
    printf("??? %s\n",temp.readPtr());
    puts("+++ replace() finished OK!");
  }

  printf("Test %d: allocation logics: obtain a write pointer\n",++tests);
  char *wp = temp.writePtr( res );
  if((res != ERR_NO_ERROR) || (wp == is) || (wp != temp.writePtr())){
    errors++;
    printf("*** Error: writePtr() returned 0x%.4x\n",(int)res);
    printf("*** Error: pointers are %p != %p == %p\n",is,wp,temp.readPtr());
  } else {
    printf("??? %s\n",temp.readPtr());
    puts("+++ writePtr() finished OK!");
  }
  
  printf("Test %d: allocation logics: copy CTOR\n",++tests);
  cwtBuffer *tt2 = new cwtBuffer(temp);
  if(tt2->readPtr() != temp.readPtr()){
    errors++;
    printf("*** Error: pointers are %p == %p\n",tt2->readPtr(),temp.readPtr());
  } else {
    printf("??? %s\n",tt2->readPtr());
    puts("+++ copy CTOR() finished OK!");
  }
    
  printf("Test %d: allocation logics: obtain a write pointer\n",++tests);
  wp = temp.writePtr( res );
  if((res != ERR_NO_ERROR) || (wp == tt2->readPtr()) || (wp != temp.writePtr())){
    errors++;
    printf("*** Error: writePtr() returned 0x%.4x\n",(int)res);
    printf("*** Error: pointers are %p != %p == %p\n",tt2->readPtr(),wp,temp.readPtr());
  } else {
    printf("??? %s\n",temp.readPtr());
    puts("+++ writePtr() finished OK!");
  }

  printf("Test %d: allocation logics: obtain another write pointer\n",++tests);
  wp = tt2->writePtr( res );
  // this is the only reference, should not branch for writePtr() !
  if((res != ERR_NO_ERROR) || (wp != tt2->readPtr()) || (wp != tt2->writePtr()) || (wp == temp.readPtr())){
    errors++;
    printf("*** Error: writePtr() returned 0x%.4x\n",(int)res);
    printf("*** Error: pointers are %p == %p != %p\n",tt2->readPtr(),wp,temp.readPtr());
  } else {
    printf("??? %s\n",temp.readPtr());
    puts("+++ writePtr() finished OK!");
  }
  
  printf("Test %d: new and delete\n",++tests);
  wtBuffer<double> *db = new wtBuffer<double>;
  const size_t recs2allocate = 1000;
  db->allocateRecs(recs2allocate);
  db->writePtr();
  size_t dbbs = db->byte_size();
  delete db;
  puts("+++ new and delete finished OK!");

  printf("Test %d: allocateRecs()\n",++tests);
  // has been done, but we'll check the size here
  if(dbbs != recs2allocate * sizeof(double)){
    ++errors;
    printf("*** Error: allocateRecs() allocated %u bytes instead of %u\n",
	   dbbs, recs2allocate * sizeof(double));
  } else {
    puts("+++ wtBuffer<double>::allocateRecs() finished OK!");
  }

  printf("\n%d tests completed with %d errors.\n",tests,errors);
  printf("Used version: %s\n",temp.VersionTag());

  return 0;  
}

#endif //TEST
