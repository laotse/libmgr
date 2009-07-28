/*
 *
 * Doubly linked lists
 *
 * (c) 2005 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: lists.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  mgrList - generic doubly linked list
 *  mgrNode - nodes for this list
 *
 * This defines the values:
 *
 */

#ifndef _UTIL_LISTS_H_
#define _UTIL_LISTS_H_

#include <mgrError.h>
#include <unistd.h>

#ifndef NULL
# define NULL 0L
#endif

class __mgrNode {
  friend class __mgrList;

  __mgrNode *next;
  __mgrNode *last;

 public:
  __mgrNode();
  __mgrNode(const __mgrNode&);  
  __mgrNode& operator=(const __mgrNode&);
  __mgrNode *succ(void);
  __mgrNode *pred(void);
  bool isFirst(void);
  bool isLast(void);
  bool isLinked(void);
  m_error_t prepend(__mgrNode *);
  m_error_t postpend(__mgrNode *);
  m_error_t unlink(void);
  m_error_t linkage(void);
  m_error_t swap(__mgrNode *);
  int buddy(__mgrNode *);
};

template <class N> class mgrNode : public __mgrNode {

 public:
  inline mgrNode() {__mgrNode::__mgrNode();};
  inline mgrNode(const mgrNode<N>& n) {__mgrNode::__mgrNode((const __mgrNode&) n);};
  inline N *succ(void) {return (N *) __mgrNode::succ();};
  inline N *pred(void) {return (N *) __mgrNode::pred();};
};

/*
 * This is what does the job on abstract __mgrNode
 *
 */

class __mgrList {  
  __mgrNode head;
  __mgrNode tail;
  m_error_t clear(bool);
  m_error_t sort(__mgrNode *, __mgrNode *);
  int (*fcmp)(__mgrNode *, __mgrNode *);

 public:
  __mgrList();
  __mgrList::__mgrList(const __mgrList&);
  bool isEmpty();
  __mgrNode *getHead();
  __mgrNode *getTail();
  m_error_t isValid();
  m_error_t addHead(__mgrNode *);
  m_error_t addTail(__mgrNode *);
  m_error_t clear(void);
  m_error_t purge(void);
  size_t count(void);
  m_error_t set_cmp(int (*)(__mgrNode *,__mgrNode *));
  inline m_error_t sort(void) {return sort(getHead(),getTail());};
  __mgrNode *operator[](size_t);
  const char *VersionTag(void);
  int cmp(__mgrNode *, __mgrNode *);
  bool isSequence(__mgrNode *, __mgrNode *);
};

/*
 * This is the class to use finally
 * <class N> must be derived from mgrNode
 *
 */

template <class N> class mgrList : public __mgrList {
  
 public:
  inline mgrList() {__mgrList::__mgrList();};
  mgrList(const mgrList<N>&);
  inline N *getHead() {return (N *) __mgrList::getHead();};
  inline N *getTail() {return (N *) __mgrList::getTail();};
  inline N *operator[](size_t i) {return (N *) __mgrList::operator[](i);};
};

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

/*
 * A list walker
 *
 */

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

#endif // _UTIL_LISTS_H_
