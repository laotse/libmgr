/*
 *
 * Extensible write-through buffer
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: RingBuffer.cpp,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  wtBuffer  - an extensible auto-allocation buffer
 *              with write-through capability
 *
 * This defines the values:
 *  DEFAULT_WTBUFFER_CHUNK
 *
 */

/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <stdio.h>

//#define DEBUG
#define DEBUG_ALLOCATE 1
#define DEBUG (DEBUG_ALLOCATE)
#include <mgrDebug.h>

#include "RingBuffer.h"

using namespace mgr;

int main(int argc, char *argv[]){
  m_error_t res;
  size_t tests, errors;
  tests = errors = 0;
  const size_t testSize = 10;

  try {
    printf("Test %d: RingBuffer::CTOR\n",++tests);
    RingBuffer<int> rbf(testSize);
    int fail = 0;
    if(rbf.capacity() != testSize) ++fail;
    if(rbf.size() != 0) ++fail;
    if(rbf.remain() != testSize) ++fail;
    if(!rbf.isEmpty()) ++fail;
    if(rbf.isFull()) ++fail;
    if(fail){
      errors++;
      printf("*** Error: RingBuffer::CTOR(%d) wrongly initialised - "
	     "capacity()=%u size()=%u remain()=%u\n",
	     testSize, rbf.capacity(), rbf.size(), rbf.remain());
    } else {    
      puts("+++ RingBuffer::CTOR() finished OK!");
    }

    printf("Test %d: RingBuffer::push()\n",++tests);
    fail = 0;
    for(size_t i=1; i<=testSize; ++i){
      res = rbf.push(i);
      bool round = false;
      if(rbf.capacity() != testSize) { ++fail; round = true; }
      if(rbf.size() != i) { ++fail; round = true; }
      if(rbf.remain() != (testSize - i)) { ++fail; round = true; }
      if(rbf.isEmpty()) { ++fail; round = true; }
      if(rbf.isFull() && (i != testSize)) { ++fail; round = true; }
      if((i == testSize) && !rbf.isFull()) { ++fail; round = true; }
      if(res != ERR_NO_ERROR) { ++fail; round = true; }
      if(round){
	printf("*** Error: RingBuffer::push() lost bookkeeping at %d - "
	       "capacity()=%u size()=%u remain()=%u, "
	       "isEmpty()=%s, isFull()=%s\n",
	       i, rbf.capacity(), rbf.size(), rbf.remain(),
	       (rbf.isEmpty())? "true" : "false",
	       (rbf.isFull())? "true" : "false");
      }
    }
    if(fail){
      errors++;
      printf("*** Error: RingBuffer::push() bookkeeping failed!\n");
    } else {    
      puts("+++ RingBuffer::push() finished OK!");
    }

    printf("Test %d: RingBuffer::push() overlow\n",++tests);
    res = rbf.push(2 * testSize);
    if(res != ERR_CANCEL){
      ++errors;
      printf("*** Error: RingBuffer::push() bookkeeping failed - 0x%.4x\n",
	     (int)res);
    } else {
      puts("+++ RingBuffer::push() finished OK!");
    }

    printf("Test %d: RingBuffer::pop()\n",++tests);
    fail = 0;
    for(size_t i=1; i<=testSize; ++i){
      const int p = rbf.pop(&res);
      bool round = false;
      if(rbf.capacity() != testSize) { ++fail; round = true; }
      if(rbf.size() != testSize - i) { ++fail; round = true; }
      if(rbf.remain() != i) { ++fail; round = true; }
      if(rbf.isFull()) { ++fail; round = true; }
      if(rbf.isEmpty() && (i != testSize)) { ++fail; round = true; }
      if((i == testSize) && !rbf.isEmpty()) { ++fail; round = true; }
      if(p != (int)i) { ++fail; round = true; }
      if(res != ERR_NO_ERROR) { ++fail; round = true; }
      if(round){
	printf("*** Error: RingBuffer::push() lost bookkeeping at %d - "
	       "capacity()=%u size()=%u remain()=%u, "
	       "isEmpty()=%s, isFull()=%s, pop()=%d\n",
	       i, rbf.capacity(), rbf.size(), rbf.remain(),
	       (rbf.isEmpty())? "true" : "false",
	       (rbf.isFull())? "true" : "false", p);
      }
    }
    if(fail){
      errors++;
      printf("*** Error: RingBuffer::pop() bookkeeping failed!\n");
    } else {    
      puts("+++ RingBuffer::pop() finished OK!");
    }

    printf("Test %d: RingBuffer::pop() overlow\n",++tests);
    rbf.pop(&res);
    if(res != ERR_CANCEL){
      ++errors;
      printf("*** Error: RingBuffer::pop() bookkeeping failed - 0x%.4x\n",
	     (int)res);
    } else {
      puts("+++ RingBuffer::pop() finished OK!");
    }

    // create some test-data
    int *cnt = (int *)malloc((testSize + 1) * sizeof(int));
    if(!cnt) mgrThrowExplain(ERR_MEM_AVAIL,"allocate cnt");
    int *rcv = (int *)malloc((testSize + 1) * sizeof(int));
    if(!rcv) mgrThrowExplain(ERR_MEM_AVAIL,"allocate rcv");
    for(int i=0;i<=(int)testSize;++i) cnt[i] = i + 1;
    const size_t fh = testSize / 2 + 1;
    const size_t sh = testSize - fh;
    
    printf("Test %d: RingBuffer::write()\n",++tests);
    fail = 0;
    size_t l = fh;
    res = rbf.write( cnt, l );
    if(res != ERR_NO_ERROR) ++fail;
    if(rbf.size() != fh ) ++fail;
    if(rbf.remain() != sh ) ++fail;
    if(rbf.isFull()) ++fail;
    if(rbf.isEmpty()) ++fail;
    if(l != fh) ++fail;
    if(fail){
      errors++;
      printf("*** Error: RingBuffer::write()=0x%.4x - "
	     "capacity()=%u size()=%u remain()=%u, "
	     "isEmpty()=%s, isFull()=%s, %u->%u\n",
	     (int)res, rbf.capacity(), rbf.size(), rbf.remain(),
	     (rbf.isEmpty())? "true" : "false",
	     (rbf.isFull())? "true" : "false",fh,l);
    } else {    
      puts("+++ RingBuffer::write() finished OK!");
    }

    printf("Test %d: RingBuffer::write() overflow\n",++tests);
    fail = 0;
    l = fh;
    res = rbf.write( cnt, l );
    if(res != ERR_CANCEL) ++fail;
    if(rbf.size() != testSize ) ++fail;
    if(rbf.remain() != 0 ) ++fail;
    if(!rbf.isFull()) ++fail;
    if(rbf.isEmpty()) ++fail;
    if(l != sh) ++fail;
    if(fail){
      errors++;
      printf("*** Error: RingBuffer::write()=0x%.4x - "
	     "capacity()=%u size()=%u remain()=%u, "
	     "isEmpty()=%s, isFull()=%s, %u->%u\n",
	     (int)res, rbf.capacity(), rbf.size(), rbf.remain(),
	     (rbf.isEmpty())? "true" : "false",
	     (rbf.isFull())? "true" : "false",fh,l);
    } else {    
      puts("+++ RingBuffer::write() finished OK!");
    }

    printf("Test %d: RingBuffer::read()\n",++tests);
    fail = 0;
    l = sh;
    res = rbf.read( rcv, l );
    if(res != ERR_NO_ERROR) ++fail;
    if(rbf.size() != fh ) ++fail;
    if(rbf.remain() != sh ) ++fail;
    if(rbf.isFull()) ++fail;
    if(rbf.isEmpty()) ++fail;
    if(l != sh) ++fail;
    if(memcmp(cnt,rcv,sh*sizeof(int))) ++fail;
    if(fail){
      errors++;
      printf("*** Error: RingBuffer::read()=0x%.4x - "
	     "capacity()=%u size()=%u remain()=%u, "
	     "isEmpty()=%s, isFull()=%s, %u->%u\n",
	     (int)res, rbf.capacity(), rbf.size(), rbf.remain(),
	     (rbf.isEmpty())? "true" : "false",
	     (rbf.isFull())? "true" : "false",fh,l);
      printf("*** Contents:");
      for(size_t i=0;i<l;++i)
	printf(" %d:%d",cnt[i],rcv[i]);
      printf("\n");
    } else {    
      puts("+++ RingBuffer::read() finished OK!");
    }

    printf("Test %d: RingBuffer::read() overflow\n",++tests);
    fail = 0;
    l = testSize;
    res = rbf.read( rcv, l );
    if(res != ERR_CANCEL) ++fail;
    if(rbf.size() != 0 ) ++fail;
    if(rbf.remain() != testSize ) ++fail;
    if(rbf.isFull()) ++fail;
    if(!rbf.isEmpty()) ++fail;
    if(l != fh) ++fail;
    int q=sh;
    for(size_t i=0;i<l;++i){
      if(++q > (int)fh) q = 1;
      if(q != rcv[i]) ++fail;
    }
    if(fail){
      errors++;
      printf("*** Error: RingBuffer::read()=0x%.4x - "
	     "capacity()=%u size()=%u remain()=%u, "
	     "isEmpty()=%s, isFull()=%s, %u->%u\n",
	     (int)res, rbf.capacity(), rbf.size(), rbf.remain(),
	     (rbf.isEmpty())? "true" : "false",
	     (rbf.isFull())? "true" : "false",fh,l);
      printf("*** Contents:");
      q=sh;
      for(size_t i=0;i<l;++i){
	if(++q > (int)fh) q = 1;
	printf(" %d:%d",q,rcv[i]);
      }
      printf("\n");
    } else {    
      puts("+++ RingBuffer::read() finished OK!");
    }

    printf("\n%d tests completed with %d errors.\n",tests,errors);
  }
  // STL and new mgr Exceptions
  catch(std::exception& e){
    puts(e.what());
  }
  // old m_error_t type exceptions
  catch(int err){
    printf("*** Exception thrown: 0x%.4x\n",err);
  }

  return 0;
}

#endif // TEST
