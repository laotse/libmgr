/*! \file dlist-operations.h
    \brief Double-Linked lists - Common Operations

    This file is included into "dlist.h" multiple
    times. It defines the common operations on a
    list object.

    This appears to be the only possibility to
    have the common code only once in source. Since
    the various list objects are different in their
    access model to the actual _List, the list operations
    could either immediately inherit from the list base
    class, which would require multiple identical
    operation classes, or could be inherited, but then
    the list access would be required as a virtual
    function, which would destroy the inlining capability
    of the static lists.

    Therefore, we add the operations to each list class
    by immediate inclusion of code.

    \version  $Id: dlist-operations.h,v 1.4 2008-05-15 20:58:24 mgr Exp $
    \author Dr. Lars Hanke
    \date 2007
*/


//! check for empty list
inline bool empty() const { return getList().empty(); }
//! count elements
size_t size() const {
  size_t c = 0;
  for( Iterator n = begin(); n != end(); ++n, ++c);
  return c;
}

//! Iterator boundary start forward
const Node *begin() const {
  return getList().Head().Next;
}
//! Iterator boundary end forward
const Node *end() const {
  return getList().pTail();
}
//! Iterator boundary start reverse
const Node *rbegin() const {
  return getList().Tail().Prev;
}
//! Iterator boundary end reverse
const Node *rend() const {
  return getList().pHead();
}

/*
 * The push_back(), back(), etc. interface must rely on Node
 * since DList does not know about content.
 */

//! Get the last Node in sequence
Node *back() const {
  if(empty()) return NULL;
  return getList().Tail().Prev;
}
//! Get the first Node in sequence
Node *front() const {
  if(empty()) return NULL;
  return getList().Head().Next;
}
//! Append Node to end of sequence
Node *push_back( const Node *n ){
  Iterator it = end();
  return it.insert( n );
}
//! Insert Node in front of sequence
Node *push_front( const Node *n ){
  Iterator it = begin();
  return it.insert( n );
}
//! Remove and return last Node in sequence
/*! \return Pointer to unlinked last Node or NULL
  
    The Node is unlinked from the DList and
    heapMark()'ed. It is the duty of the client
    to delete this node in order to avoid memory
    leaks.
*/
Node *pop_back() {
  Iterator it = rbegin();
  return it.remove();
}
//! Remove and return first Node in sequence
/*! \return Pointer to unlinked first Node or NULL
  
    The Node is unlinked from the DList and
    heapMark()'ed. It is the duty of the client
    to delete this node in order to avoid memory
    leaks.
*/
Node *pop_front() {
  Iterator it = begin();
  return it.remove();
}
