/*
 *
 * Binary trees
 *
 * (c) 2005 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: bintree.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  mgrList - generic doubly linked list
 *  mgrNode - nodes for this list
 *
 * This defines the values:
 *
 */

#ifndef _UTIL_BINTREE_H_
#define _UTIL_BINTREE_H_

#include <mgrError.h>
#include <unistd.h>

#ifndef NULL
# define NULL 0L
#endif

class __binLeaf {
  friend class __binTree;

  __binLeaf *parent;
  __binLeaf *child[2];

 public:
  inline __binLeaf() {parent = child[0] = child[1] = NULL;};
  inline __binLeaf(const __binLeaf&) {};  
  inline __binLeaf& operator=(const __binLeaf&) {};
  inline __binLeaf *left(void) {return child[0];};
  inline __binLeaf *right(void) {return child[1];};
  inline __binLeaf *up(void) {return (parent && parent->parent)?parent:NULL;};
  bool isRoot(void);
  m_error_t swap(__binLeaf *);
  m_error_t add(__binLeaf *,unsigned const char);
  m_error_t __binLeaf::insert(__binLeaf *,unsigned const char);
  unsigned char isLeaf(void);
  bool isLinked(void);
};

template <class N> class binLeaf : public __binLeaf {

 public:
  inline binLeaf() {__binLeaf::__binLeaf();};
  inline binLeaf(const binLeaf<N>& n) {__binLeaf::__binLeaf((const __binLeaf&) n);};
  inline N *left(void) {return (N *) __binLeaf::left();};
  inline N *right(void) {return (N *) __binLeaf::right();};
  inline N *up(void) {return (N *) __binLeaf::up();};
  inline N *root(void) {return (N *) __binLeaf::root();};
};

/*
 * This is what does the job on abstract __binLeaf
 *
 */

class __binTree {  
  __binLeaf root;
  // m_error_t clear(bool);
  // m_error_t sort(__mgrNode *, __mgrNode *);
  int (*fcmp)(__binLeaf *, __binLeaf *);
  __binLeaf *next(__binLeaf *,bool);

 public:
  __binTree();
  __binTree::__binTree(const __binTree&);
  inline bool isEmpty() {return (root.child[0] == NULL);};
  inline __binLeaf *getRoot() {return (root.child[0]);};
  m_error_t isValid();
  bool isParent(__binLeaf *, __binLeaf *);
  inline m_error_t addHead(__binLeaf *l) {return addLeaf(l,NULL,false);};
  m_error_t addLeaf(__binLeaf *,__binLeaf *, bool);
  m_error_t insertLeaf(__binLeaf *,__binLeaf *, bool);
  /*
  m_error_t clear(void);
  m_error_t purge(void);
  size_t count(void);
  size_t depth(void);
  m_error_t rotate(__binLeaf *, bool);
  m_error_t set_cmp(int (*)(__binLeaf *,__binLeaf *));
  int cmp(__binLeaf *, __binLeaf *);
  inline m_error_t sort(void) {return sort(getHead(),getTail());};
  */
  const char *VersionTag(void);
};

/*
 * This is the class to use finally
 * <class N> must be derived from __binLeaf
 *
 */

template <class N> class binTree : public __binTree {
  
 public:
  inline binTree() {__binTree::__binTree();};
  binTree(const binTree<N>&);
  inline N *getRoot() {return (N *) __binTree::getRoot();};
};

/*
// copy constructor
template <class N> mgrList<N>::mgrList(const mgrList<N>& l){
  // fixme - this is a brute workaround to get rid of the const qualifier!
  // should be: N *n = l.getHead();
  // but that won't compile
  N *n = ((mgrList<N> *)&l)->getHead();
  N *p;

#ifdef _UTIL_DEBUG_H_
  // include debugging message, if and only if, the debugging system
  // util/debug.h is included, the latter will include this according
  // to the settings of DEBUG and DEBUG_CTOR
  xpdbg(CTOR,"... called copy constructor for mgrList at 0x%lx\n",this);
#endif

  while(n->succ()){
    p = new N;
    *p = *n;
    this->addTail(p);
    n = n->succ();
  }
}

*/

/*
 * A list walker
 *
 */

// We must find something to make it decide for left or right

/*
class __mgrListWalk {
  __mgrList *list;
  __mgrNode *current;

 public:
  __mgrListWalk(__mgrList *);
  __mgrNode *rewind(void);
  __mgrNode *operator()(void);
};

template <class N> class mgrListWalk : private __mgrListWalk {
 public:
  mgrListWalk(void *l) : __mgrListWalk((__mgrList *)l) {};
  inline N *rewind(void) {return (N *)  __mgrListWalk::rewind();};
  inline N *operator()(void) {return (N *)  __mgrListWalk::operator()();};
};
*/

#endif // _UTIL_LISTS_H_
