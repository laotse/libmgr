/*! \file ttree.cpp
    \brief Test cases for TTree and TTreeNode templates

    \version  $Id: ttree.cpp,v 1.5 2008-05-15 20:58:25 mgr Exp $
    \author Dr. Lars Hanke
    \date 2007
*/

//! Dump tree contents - currently not used
#define DEBUG_DUMP  1
//! Mark progress in program - currently not used
#define DEBUG_MARK  2
//! Report allocation - noisy and instructive
#define DEBUG_ALLOC 4
//! Current debugging mode
#define DEBUG ( DEBUG_ALLOC )
#ifdef DEBUG
# include <stdio.h>
#endif
#include <mgrDebug.h>

#include "ttree.h"

using namespace mgr;

/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <string>
#include <iostream>
#include <fstream>

using namespace std;

const string &safeString( const string *s ){
  static const string defstr = "(nil)";
  if(s) return *s;
  return defstr;
}

/*
 * Direct payload
 *
 */

typedef class TTree<string> Tree;
typedef class Tree::TTreeIterator TreeIter;

void dump( TreeIter& t ){
  cout << "Dumping Tree" << endl;
  string *s = t.root();
  size_t depth = t.depth();
  while(s){
    cout << depth << ": " << *s << endl;
    s = t.iterate( &depth );
  }
  cout << "Tree dumped!" << endl;
}

/*
 * Indirect payload and inheritance
 *
 */

class myClass : public TTreeNodeBase {
protected:
  string content;

public:
  myClass( const char *s ) : content( s ) {
    xpdbg(ALLOC,"### Created %p from string %p: \"%s\"\n",this,s,content.c_str());
  }
  myClass( const string& s ) : content( s ) {}
  myClass( const myClass& c ) : content( c.content ) {  
    // cout << "### copy CTOR called for " << content << endl;
  }
  myClass( const TTreeNodeBase& c )
    : content( static_cast<const myClass&>(c).content ) {  }
  virtual ~myClass() { 
    xpdbg(ALLOC,"### Destroy %p: \"%s\"\n",this,content.c_str());
  }

  const string& str() const { return content; }

  virtual TTreeNodeBase *clone() const {
    TTreeNodeBase *n = static_cast<TTreeNodeBase *>( new myClass( *this ) );
    xpdbg(ALLOC,"### %p=clone(%p): \"%s\"\n",n,this,content.c_str());
    return n;
  }
};

typedef class TTree<myClass,true> ITree;
typedef class ITree::TTreeIterator ITreeIter;

void dump( ITreeIter& t ){
  cout << "Dumping ITree" << endl;
  myClass *s = t.root();
  size_t depth = t.depth();
  while(s){
    cout << depth << ": " << s->str() << endl;
    try {
      s = t.iterate( &depth );
    }
    catch( std::exception& e ){
      puts(e.what());
    }
  }
  cout << "ITree dumped!" << endl;
}

/*
 * Main program
 *
 */

int main( int argc, char * const argv[] ){
  Tree myTreeBase;
  TreeIter myTree(myTreeBase);

  myTree.appendSequence("Ebene 1, erste");
  myTree.appendSequence("Ebene 1, zweite",true);
  myTree.appendChild("Ebene 2, erste",true);
  myTree.appendSequence("Ebene 2, zweite");
  cout << "Upwards to: " << *(myTree.parent()) << endl;
  myTree.appendSequence("Ebene 1, dritte");

  dump( myTree );

  cout << "Shallow Copy" << endl;
  Tree myCopy( myTreeBase );
  TreeIter cpIter( myCopy );
  dump( cpIter );

  cout << "Deep Copy" << endl;
  Tree myDeep;
  myDeep.copy( myCopy );
  TreeIter dpIter( myDeep );
  dump( dpIter );

  ITree myIBase;
  ITreeIter IIter( myIBase );

  IIter.appendSequence("Ebene 1, erste");
  IIter.appendSequence("Ebene 1, zweite",true);
  IIter.appendChild("Ebene 2, erste",true);
  IIter.appendSequence("Ebene 2, zweite");
  try {
    cout << "Upwards to: " << IIter.parent()->str() << endl;
  }
  catch( std::exception& e ){
    puts(e.what());
  }
  IIter.appendSequence("Ebene 1, dritte");

  dump( IIter );
  
  cout << "Shallow Copy indirect" << endl;
  ITree myICopy( myIBase );
  ITreeIter cpIIter( myICopy );
  dump( cpIIter );

  cout << "Deep Copy indirect" << endl;
  ITree myIDeep;
  myIDeep.copy( myICopy );
  ITreeIter dpIIter( myIDeep );
  dump( dpIIter );
}

#endif // TEST
