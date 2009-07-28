/*! \file HTree.cpp
    \brief Hierarchical tree aka Sequence of Scopes

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

    This files defines hierarchical trees or sequences of sequences
    or nested scopes, or ... This is the native data structure of
    TLV coded data, e.g. BER, tagged data, e.g. XML, or scoped 
    data, e.g. program code.

    \version  $Id: htree-sol.cpp,v 1.6 2008-05-15 20:58:24 mgr Exp $
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

#include "htree-sol.h"
#include "htree-sol.tag"

using namespace mgr::sol;
using namespace mgr;

const char *_HTreeGeneric::VersionTag() {
  return _VERSION_;
}

const _HTreeGeneric::Node& _HTreeGeneric::Iterator::begin() const {
  if( path.size() < 2 )
    mgrThrowExplain( ERR_PARAM_RANG, "sol::HTree::Iterator begin of non-sequence requested" );
  _iterInt i = path[path.size() - 2];
  return *(static_cast<const Node*>(i->Children.begin()));
}
const _HTreeGeneric::Node& _HTreeGeneric::Iterator::end() const {
  if( path.size() < 2 )
    mgrThrowExplain( ERR_PARAM_RANG, "sol::HTree::Iterator end of non-sequence requested" );
  _iterInt i = path[path.size() - 2];
  return *(static_cast<const Node*>(i->Children.end()));
}
_HTreeGeneric::Iterator& _HTreeGeneric::Iterator::child() {
  if( path.back()->Children.empty() )
    mgrThrowExplain( ERR_PARAM_RANG, "sol::HTree::Iterator child of leaf node requested" );
  _iterInt i( path.back()->Children.begin() );
  path.push_back( i );
  return *this;
}
_HTreeGeneric::Iterator& _HTreeGeneric::Iterator::parent() {
  if(path.size() < 2) 
    mgrThrowExplain( ERR_PARAM_RANG, "sol::HTree::Iterator parent of root requested" );
  path.pop_back();
  return *this;
}

_HTreeGeneric::Iterator& _HTreeGeneric::Iterator::root() {
  if(!path.size())
    mgrThrowExplain( ERR_PARAM_TYP, "sol::HTree::Iterator root of uninitialized requested" );
  _iterInt t = path[0];
  path.clear();
  path.push_back( t );
  return *this;
}
_HTreeGeneric::Node *_HTreeGeneric::Iterator::iterate( size_t *dpth )  {
  if(hasChildren()){
    child();
    if(dpth) *dpth = depth();
    return static_cast<Node*>(path.back().operator->());
  } 
  if(hasNext()) {
    ++(*this);
    if(dpth) *dpth = depth();
    return static_cast<Node*>(path.back().operator->());
  }
  // Neither has siblings nor children
  bool stop = false;
  do {
    // pdbg("### Resume: %p (%d)\n", path.back(), path.size());
    if(!hasParent()) {
      stop = true;
      break;
    }
    parent();
    // pdbg("### Resume parent: %p (%d)\n", path.back(), path.size());
    if(!hasNext()) break;
    ++(*this);
    stop = true;
  } while(!stop);
  if(!hasParent()){
    if(dpth) *dpth = 0;
    return NULL;
  }
  if(dpth) *dpth = depth();
  return static_cast<Node*>(path.back().operator->());
}


/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <cstdio>

class Test : public HTree::Node {
public:
  int Value;
  Test( const int& i = -1) : Value( i ) {}
  Test( const Test& t ) : Value( t.Value ) {}
  virtual ~Test() {}
  virtual Cloneable *clone() const { return new Test( *this ); }
};

int main(int argc, char *argv[]){
  // m_error_t res;
  size_t tests, errors;

  tests = errors = 0;

  try {
    printf("Test %d: HTree.CTOR\n",++tests);
    HTree list;
    puts("+++ HTree.CTOR() finished OK!");

    printf("Test %d: HTree.empty()\n",++tests);
    if(!list.empty()){
      ++errors;
      printf("*** Error: New created tree is not empty!\n");
    } else {
      puts("+++ HTree.empty() finished OK!");
    }

    printf("Test %d: HTree::iterator.CTOR\n",++tests);
    HTree::iterator<Test> it;
    puts("+++ HTree::iteartor.CTOR() finished OK!");

    printf("Test %d: HTree::iterator assign\n",++tests);
    it = list.root();
    puts("+++ HTree::iterator assign finished OK!");

    printf("Test %d: HTree::iterator.insertChild()\n",++tests);
    Test tt(0);
    Test *pt = it.insertChild( tt );
    if( !pt || (pt == &tt)){
      ++errors;
      printf("*** Error: insertChild() returned %p for item %p\n",pt,&tt);
    } else {
      puts("+++ insertChild() finished OK!");
    }

    printf("Test %d: HTree.empty()\n",++tests);
    if(list.empty()){
      ++errors;
      printf("*** Error: Tree is empty after insertChild()\n");
    } else {
      puts("+++ HTree.empty() finished OK!");
    }

    printf("Test %d: iterate\n",++tests);
    it.root();
    size_t dpt = 0;
    ssize_t ctr = 1;
    while( (pt = it.iterate( &dpt )) ){
      printf("??? %u: %d\n",dpt,pt->Value);
      --ctr;
    }
    if( ctr ){
      ++errors;
      printf("*** Error: wrong number of elements - ctr: %d\n",ctr);
    } else {
      puts("+++ iterate() finished OK!");
    }

    printf("\n%d tests completed with %d errors.\n",tests,errors);
    printf("Used version: %s\n",list.VersionTag());
  }
  catch(const std::exception& e){
    printf("*** Exception thrown: %s\n",e.what());
  }

  return 0;  
}

#endif //TEST
