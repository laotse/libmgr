/*! \file dlist.cpp
    \brief Double-Linked lists

    Standard Object Libarary \b libsol.a aims at implementing
    standard data structures by abstract code as used with 
    C-libraries instead of templates as commen with C++ STL.
    There are a couple of major advantages with SOL as compared to STL:
    \li You can have containers for base classes, which will
    contain inherited classes, without adding another level of
    indirection.
    \li Code for a particular container class is created only
    once. This is different to e.g. std::list<int> and 
    std::list<double>, which will produce the std:list base
    code two times.
    \li The base code can be pre-compiled into a libarary.
    The compiler is not burdened with template mangling, which
    drastically enhances compilation resources.

    This file defines double linked lists and is a
    replacement of std::list. If you are using only a few
    different data types in lists use std::list. If you
    have to use polymorphic list items, use DList.

    \version  $Id: dlist.cpp,v 1.6 2008-05-15 20:58:24 mgr Exp $
    \author Dr. Lars Hanke
    \date 2007
*/

//#define DEBUG
#define DEBUG_ALLOC (1 << 0)
#define DEBUG (DEBUG_ALLOC)
#include <mgrDebug.h>

#ifdef DEBUG
# include <cstdio>
#endif

#include "dlist.h"
#include "dlist.tag"

using namespace mgr::sol;
using namespace mgr;

const char *_DListGeneric::VersionTag() {
  return _VERSION_;
}

/*! \param j Node to insert
    \return New node or NULL, if n = Tail or otherwise invalid
*/
_DListGeneric::Node *_DListGeneric::Iterator::insert( const Node *j ) {	
  if(!n || !(n->Prev)) return NULL;
  if( !j ) return NULL;
  Node *i = NULL;
  if(!(j->isMarked())){
    i = static_cast<Node *>(j->clone());
    if(!i) return NULL;
  } else i = const_cast<Node*>(j);
  i->Prev = n->Prev;
  n->Prev->Next = i;
  n->Prev = i;
  i->Next = n;
  return i;
}

/*! \return Pointer to removed Node or NULL
  
    After removing the Node the Iterator
    is advanced to the next node towards
    the end of the DList.

    The Node removed is unlinked and
    heapMark()'ed, but not deleted.
*/
_DListGeneric::Node *_DListGeneric::Iterator::remove() {
  if(!n || !(n->isLinked())) return NULL;
  n->Next->Prev = n->Prev;
  n->Prev->Next = n->Next;
  Node *r = n;
  n = n->Next;
  // Make a heap mark
  r->Next = NULL;
  r->Prev = r;
  
  return r;
}

void _DListGeneric::_ListAnchor::clear() {
  Node *n = Head().Next;
  while(n->Next){
    n = n->Next;
    delete n->Prev;
  }
  init();
}

void _DListGeneric::_ListAnchor::deepCopy( const _ListAnchor& l ){
  clear();
  Node *p = static_cast<Node*>(&(Anchor.head._Head));
  const Node *s = l.Head().Next;
  try {
    while(s->Next){
      Node *n = static_cast<Node *>(s->clone());
      if(!n) mgrThrowExplain( ERR_PARAM_NULL, "_ListAnchor deep copy" );
      p->Next = n;
      n->Prev = p;
      p = n;
      s = s->Next;
    }
  }
  catch(...){
    fixTail(p);
    throw;
  }
  fixTail(p);
}

/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <cstdio>

class Test : public DList::Node {
public:
  int Value;
  Test( const int& i = -1) : Value( i ) {}
  Test( const Test& t ) : Value( t.Value ) {}
  virtual ~Test() {}
  virtual Cloneable *clone() const { return new Test( *this ); }
};

int main(int argc, char *argv[]){
  m_error_t res;
  size_t tests, errors;

  tests = errors = 0;

  try {
    printf("Test %d: DList.CTOR\n",++tests);
    DList list;
    puts("+++ DList.CTOR() finished OK!");

    printf("Test %d: DList.empty()\n",++tests);
    if(!list.empty()){
      ++errors;
      printf("*** Error: New created list is not empty!\n");
    } else {
      puts("+++ DList.empty() finished OK!");
    }

    printf("Test %d: DList.size()\n",++tests);
    size_t sz = list.size();
    if(sz != 0){
      ++errors;
      printf("*** Error: New created list too large: %u\n",sz);
    } else {
      puts("+++ DList.size() finished OK!");
    }

    printf("Test %d: DList::iterator.CTOR\n",++tests);
    DList::iterator<Test> it;
    puts("+++ DList::iterator.CTOR() finished OK!");

    printf("Test %d: DList::iterator.valid()\n",++tests);
    if(it.valid()){
      ++errors;
      printf("*** Error: Void iterator is valid!\n");
    } else {
      puts("+++ DList::iterator.valid() finished OK!");
    }

    printf("Test %d: DList::Iterator.operator=()\n",++tests);
    it = list.begin();
    if(!it.valid()){
      ++errors;
      printf("*** Error: Valid iterator is void!\n");
    } else {
      puts("+++ DList::iterator.valid() finished OK!");
    }    

    printf("Test %d: DList::Iterator.insert()\n",++tests);
    Test tt(1);
    DList::Node *n = it.insert( tt );
    if(!n || ( n == &tt )){
      ++errors;
      printf("*** Error: wrong pointer returned: %p\n",n);
    } else {
      puts("+++ insert() finished OK!");
    }
    
    printf("Test %d: DList.empty()\n",++tests);
    if(list.empty()){
      ++errors;
      printf("*** Error: New created list is not empty!\n");
    } else {
      puts("+++ DList.empty() finished OK!");
    }

    printf("Test %d: DList.size()\n",++tests);
    sz = list.size();
    if(sz != 1){
      ++errors;
      printf("*** Error: New created list too large: %u\n",sz);
    } else {
      puts("+++ DList.size() finished OK!");
    }

    printf("Test %d: DList::iterator iteration\n",++tests);
    ssize_t sc = sz;
    for(it = list.begin(); it != list.end(); ++it){
      printf("??? (%d) %d\n",sc--,it->Value);
    }
    if(sc != 0){
      ++errors;
      printf("*** Error: Iteration ended at %d of %u\n",sc,sz);
    } else {
      puts("+++ DList::iterator finished OK!");
    }

    printf("Test %d: DList.Copy-CTOR\n",++tests);
    DList tl( list );
    puts("+++ DList.Copy-CTOR() finished OK!");

    printf("Test %d: push_back()\n",++tests);
    tt.Value = 2;
    n = tl.push_back( &tt );
    if(!n || (n == &tt)){
      ++errors;
      printf("*** Error: wrong pointer returned: %p\n",n);
    } else {
      puts("+++ push_back() finished OK!");
    }

    printf("Test %d: push_front()\n",++tests);
    tt.Value = -1;
    n = tl.push_front( &tt );
    if(!n || (n == &tt)){
      ++errors;
      printf("*** Error: wrong pointer returned: %p\n",n);
    } else {
      puts("+++ push_front() finished OK!");
    }

    printf("Test %d: DList.size() (across shallow assignment)\n",++tests);
    sz = list.size();
    if(sz != 3){
      ++errors;
      printf("*** Error: wrong list size: %u (3)\n",sz);
    } else {
      puts("+++ DList.size() finished OK!");
    }

    printf("Test %d: Modification test (across shallow assignment)\n",++tests);
    static_cast<Test*>(tl.back())->Value = 4;
    if(static_cast<Test*>(list.back())->Value != 4){
      ++errors;
      printf("*** Error: list not modified after assignment: back is %d\n",
	     static_cast<Test*>(list.back())->Value);
    } else {
      puts("+++ Modification test finished OK!");
    }

    printf("Test %d: DList::iterator iteration\n",++tests);
    sc = sz;
    for(it = list.begin(); it != list.end(); ++it){
      printf("??? (%d) %d\n",sc--,it->Value);
    }
    if(sc != 0){
      ++errors;
      printf("*** Error: Iteration ended at %d of %u\n",sc,sz);
    } else {
      puts("+++ DList::iterator finished OK!");
    }

    printf("Test %d: DList.branch()\n",++tests);
    res = tl.branch();
    if(res != ERR_NO_ERROR){
      ++errors;
      printf("*** Error: branch() returned %d\n",(int)res);
    } else {
      puts("+++ DList.branch() finished OK!");
    }
      
    printf("Test %d: Modification test\n",++tests);
    static_cast<Test*>(tl.back())->Value = 3;
    if(static_cast<Test*>(list.back())->Value != 4){
      ++errors;
      printf("*** Error: list modified after branch(): back is %d\n",
	     static_cast<Test*>(list.back())->Value);
    } else {
      puts("+++ Modification test finished OK!");
    }

    printf("Test %d: DList::iterator iteration\n",++tests);
    puts("??? Original List");
    for(it = list.begin(); it != list.end(); ++it){
      printf("??? %d\n",it->Value);
    }
    puts("??? branch()'ed List");
    for(it = tl.begin(); it != tl.end(); ++it){
      printf("??? %d\n",it->Value);
    }
    puts("+++ DList::iterator finished OK!");

    printf("Test %d: DList.clear()\n",++tests);    
    tl.clear();
    if(!tl.empty()){
      ++errors;
      printf("*** Error: DList not empty after clear - size %u\n",tl.size());
    } else {
      puts("+++ DList.clear() finished OK!");
    }

    /*
     * Static DList test cases
     *
     */

    printf("Test %d: SDList.CTOR\n",++tests);
    SDList sdlist;
    puts("+++ SDList.CTOR() finished OK!");

    printf("Test %d: SDList.empty()\n",++tests);
    if(!sdlist.empty()){
      ++errors;
      printf("*** Error: New created list is not empty!\n");
    } else {
      puts("+++ SDList.empty() finished OK!");
    }

    printf("Test %d: SDList.size()\n",++tests);
    sz = sdlist.size();
    if(sz != 0){
      ++errors;
      printf("*** Error: New created list too large: %u\n",sz);
    } else {
      puts("+++ SDList.size() finished OK!");
    }

    printf("Test %d: SDList::iterator.CTOR\n",++tests);
    SDList::iterator<Test> sit;
    puts("+++ SDList::iterator.CTOR() finished OK!");

    printf("Test %d: SDList::iterator.valid()\n",++tests);
    if(sit.valid()){
      ++errors;
      printf("*** Error: Void iterator is valid!\n");
    } else {
      puts("+++ SDList::iterator.valid() finished OK!");
    }

    printf("Test %d: SDList::Iterator.operator=()\n",++tests);
    sit = sdlist.begin();
    if(!sit.valid()){
      ++errors;
      printf("*** Error: Valid iterator is void!\n");
    } else {
      puts("+++ SDList::iterator.valid() finished OK!");
    }    

    printf("Test %d: SDList::Iterator.insert()\n",++tests);
    tt = 1;
    // The Node type is identical for SDList and DList
    n = sit.insert( tt );
    if(!n || ( n == &tt )){
      ++errors;
      printf("*** Error: wrong pointer returned: %p\n",n);
    } else {
      puts("+++ insert() finished OK!");
    }
    
    printf("Test %d: SDList.empty()\n",++tests);
    if(sdlist.empty()){
      ++errors;
      printf("*** Error: New created list is not empty!\n");
    } else {
      puts("+++ SDList.empty() finished OK!");
    }

    printf("Test %d: SDList.size()\n",++tests);
    sz = sdlist.size();
    if(sz != 1){
      ++errors;
      printf("*** Error: New created list too large: %u\n",sz);
    } else {
      puts("+++ SDList.size() finished OK!");
    }

    printf("Test %d: SDList::iterator iteration\n",++tests);
    sc = sz;
    for(sit = sdlist.begin(); sit != sdlist.end(); ++sit){
      printf("??? (%d) %d\n",sc--,sit->Value);
    }
    if(sc != 0){
      ++errors;
      printf("*** Error: Iteration ended at %d of %u\n",sc,sz);
    } else {
      puts("+++ SDList::iterator finished OK!");
    }

    printf("Test %d: push_back()\n",++tests);
    tt.Value = 2;
    n = sdlist.push_back( &tt );
    if(!n || (n == &tt)){
      ++errors;
      printf("*** Error: wrong pointer returned: %p\n",n);
    } else {
      puts("+++ push_back() finished OK!");
    }

    printf("Test %d: push_front()\n",++tests);
    tt.Value = -1;
    n = sdlist.push_front( &tt );
    if(!n || (n == &tt)){
      ++errors;
      printf("*** Error: wrong pointer returned: %p\n",n);
    } else {
      puts("+++ push_front() finished OK!");
    }

    printf("Test %d: SDList.size()n",++tests);
    sz = sdlist.size();
    if(sz != 3){
      ++errors;
      printf("*** Error: wrong list size: %u (3)\n",sz);
    } else {
      puts("+++ DList.size() finished OK!");
    }

    printf("Test %d: SDList::iterator iteration\n",++tests);
    sc = sz;
    for(sit = sdlist.begin(); sit != sdlist.end(); ++sit){
      printf("??? (%d) %d\n",sc--,sit->Value);
    }
    if(sc != 0){
      ++errors;
      printf("*** Error: Iteration ended at %d of %u\n",sc,sz);
    } else {
      puts("+++ SDList::iterator finished OK!");
    }

    printf("Test %d: SDList.Copy-CTOR\n",++tests);
    SDList stl( sdlist );
    puts("+++ SDList.Copy-CTOR() finished OK!");

    printf("Test %d: SDList::iterator iteration\n",++tests);
    sc = sz;
    for(sit = stl.begin(); sit != stl.end(); ++sit){
      printf("??? (%d) %d\n",sc--,sit->Value);
    }
    if(sc != 0){
      ++errors;
      printf("*** Error: Iteration ended at %d of %u\n",sc,sz);
    } else {
      puts("+++ SDList::iterator finished OK!");
    }

    printf("Test %d: SDList.clear()\n",++tests);    
    stl.clear();
    if(!tl.empty()){
      ++errors;
      printf("*** Error: SDList not empty after clear - size %u\n",stl.size());
    } else {
      puts("+++ SDList.clear() finished OK!");
    }

    printf("\n%d tests completed with %d errors.\n",tests,errors);
    printf("Used version: %s\n",list.VersionTag());
    printf("DList size: %u/%u, "
           "Node size: %u/%u, Iterator size %u/%u,\n",
	   sizeof(list), sizeof(list) / sizeof(void *),
	   sizeof(Test) - sizeof(int), 
	   (sizeof(Test) - sizeof(int)) / sizeof(void *),
	   sizeof(it), sizeof(it) / sizeof(void *));
    printf("SList size: %u/%u, Node size: %u/%u, Iterator size %u/%u,\n",
	   sizeof(sdlist), sizeof(sdlist) / sizeof(void *),
	   sizeof(Test) - sizeof(int), 
	   (sizeof(Test) - sizeof(int)) / sizeof(void *),
	   sizeof(sit), sizeof(sit) / sizeof(void *));
  }
  catch(const std::exception& e){
    printf("*** Exception thrown: %s\n",e.what());
  }

  return 0;  
}

#endif //TEST
