/*! \file dlist.h
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

    \version  $Id: dlist.h,v 1.5 2008-05-15 20:58:24 mgr Exp $
    \author Dr. Lars Hanke
    \date 2007
*/

#ifndef _SOL_DLIST_H_
//! Mark the file as included
# define _SOL_DLIST_H_

# include <Concepts.h>
# include <RefCounter.h>
# include <mgrMeta.h>

namespace mgr{ namespace sol {

  class DList;
  class SDList;

  /*! \class _DListGeneric
      \brief The generic double link list class

      This class defines the nodes and what can be done
      with nodes, i.e. the iterators. It does not contain
      the actual container class stuff. The latter is
      implemented in DList or SDList, which both inherit
      the common nodes and methods on nodes.

      This class is structured as follows:
      \arg _RawNode Only the two pointers connecting the list
      \arg Node The generic node for inheriting
      \arg Iterator Generic iterator, publically available through the
           _iterator template and its typecast templates
	   iterator, const_iterator, reverse_iterator, reverse_const_iterator
      \arg _ListAnchor The common list anchor, which is two
           overlapping nodes compressed to 3 pointers size.

      \section friends Encapsulation
      The idea is to have the client see only DList and SDList as well
      as the corresponding iterators, and of course Node to inherit from.
      Therefore all the rest are protected nested classes in _DListGeneric. 

      This rest is primarily Iterator as the base class of all iterators,
      and _ListAnchor, which is the protected base class of
      _DList and _SDList. Since the latter are nested classes of
      DList and SDList, which inherit from _DListGeneric, this
      protected base class is acesible for inheritence.

      Iterator is inherited as protected nested class to the containers
      DList and SDList. The external iterator templates are finally
      public and derived within _DListGeneric. Therefore, DList and
      SDList have identical iterators, both can use Iterator internally,
      but Iterator is not accessible by the client.

      Since we want to inherit from Node and do not want the client to
      mangle node linkage, node linkage is \b private. So, everything
      working on Node linkage directly, must be a friend of Node.

      Iterator works heavily on node linkage and is therefore also
      required as friend.
      
      Finally, the _ListAnchor maintains the ends of the list
      through _RawNode. Since _RawNode is Node pointers, _ListAnchor
      must be friend of Node.

      The friendship rules are furthermore the reason for the forward
      declarations of these classes.
  */
  class _DListGeneric {
  protected:
    class Iterator;
    class _ListAnchor;
  public:
    class Node;
    //! This is the basic Node data structure without methods or CTOR
    /*! If you ever want to fiddle around with node linkage, here
        is your weak spot. Cast a Node* to a _RawNode* and you have
	the linkage in a public struct.

	You'll still have to inherit from _DListGeneric, if you ought
	to do that. I expect you to know what you're doing then.
    */
  protected:
    struct _RawNode {
      //! Successor in list
      Node *Next;
      //! Predecessor in list
      Node *Prev;
    };
  public:
    /*! \class Node
        \brief Base class of all Nodes

        This is the base class of all nodes in DList. Actually, it's the
	only concept DList and its iterators know about. A Node has
	a successor and a predecessor, i.e. is double linked, is
	Cloneable, and will never assign linkage to another node.
	Furthermore, no inherited class has access to the private 
	node linkage.

	Node linkage can have two special values:
	\arg unlinked means that the node is not part of any list, 
             which is indicated by both successor and predecessor NULL.
	\arg marked can be set by the client, if the Node is allocated
             from heap in order to avoid copy. The pattern of the mark
	     (successor NULL, predecessor the node itself) cannot occur
	     for any linked node and can only be set for unlinked
	     nodes.
    */
    class Node : private _RawNode, public Cloneable {      
      friend class _ListAnchor;
      friend class Iterator;
    private:
      //! Alternative copy operator for internal use
      Node& operator<<( const Node& n ) { 
	Next = n.Next;
	Prev = n.Prev;
	return *this; 
      }
    public:
      //! CTOR creates unlinked node
      Node() { Next = NULL; Prev = NULL; }
      //! Copy CTOR copies contents, not linkage
      Node( const Node& ) { Next = NULL; Prev = NULL; }
      //! Assignment is done for contents, not for linkage
      Node& operator=( const Node& ) { return *this; }
      //! You must inherit from DList::Node
      virtual Cloneable *clone() const = 0;	
      //! Check, whether node is a linked node
      inline bool isLinked() const { return ((Next != NULL) && (Prev != NULL)); }
      //! Mark this node as allocated through new
      /*! When doing Iterator::insert() using a heapMark()'ed node
          It will not be copied and be owned by the list. Do not
	  delete that node afterwards.
      */
      inline m_error_t heapMark() {
	if(isLinked()) return ERR_PARAM_TYP;
	Prev = this;
	return ERR_NO_ERROR;
      }
      //! Check whether a node has the heapMark()
      inline bool isMarked() const { return ((Next == NULL) && (Prev == this)); }
      //! Swap two nodes
      /*! \param a Node to swap with
 	  \return Error code as defined in mgrError.h

	  Swap will exchange the node loactions in the
	  list, or if you like it, keep the node locations
	  and exchange the contents. Actually, swap()
	  also works across lists, but it won't work with
	  non-heap nodes. Also, swapping two unlinked, but
	  heap marked nodes will result in nothing.
	  
	  However, the function can be used to swap
	  a linked node with a heap-marked node.
	
	  Error codes returned are:
	  \arg ERR_NO_ERROR Everything is okay.
	  \arg ERR_CANCEL Attempt to swap two unlinked, but marked nodes; nothing done!
	  \arg ERR_PARAM_TYP Attempt to swap unlinked and unmarked Node.
	  \arg ERR_PARAM_NULL Any of the Node pointers is NULL
	  
      */
      m_error_t swap( Node *a ){
	if( !a ) return ERR_PARAM_NULL;
	bool mark = false;
	bool mark_me = false;
	if(!(a->isLinked())) if(a->isMarked()) mark = true; else return ERR_PARAM_TYP;
	if(!(isLinked())) if(isMarked()) mark_me = true; else return ERR_PARAM_TYP;
	if(mark && mark_me) return ERR_CANCEL;
	struct {
	  Node *Next, *Prev;
	} t;
	t.Next = a->Next; t.Prev = a->Prev;
	if(mark_me){
	  a->Prev = a;
	  a->Next = NULL;
	} else {
	  *a << *this;
	}
	if(mark){
	  Next = NULL;
	  Prev = this;
	} else {
	  Next = t.Next;
	  Prev = t.Prev;
	}
	return ERR_NO_ERROR;
      }

    };
  protected:
    //! The genric iterator class working on Node
    class Iterator {
      friend class DList;
      friend class SDList;
    protected:
      //! The node we're pointing to
      Node *n;

      //! Get contents
      Node *current() const {
	if(!n) return NULL;
	return n;
      }

      //! Move to next node
      Node *next() {
	if(!n || !(n->Next)) return NULL;
	n = n->Next;
	return (n->Next)? n : NULL;
      }
      //! Move to previous node
      Node *prev() {
	if(!n || !(n->Prev)) return NULL;
	n = n->Prev;
	return (n->Prev)? n : NULL;
      }
      
      //! Insert Node preceeding current node
      Node *insert( const Node *j );
      //! Remove this Node from list
      Node *remove();
      //! Remove a heap mark
      void unmark( Node *p ){
	if(!p || !(p->isMarked())) return;
	p->Next = NULL;
	p->Prev = NULL;
      }

    public:
      //! Setup new iterator for a node
      /*! \note We do not change the node now, and the
          client does not see the private fields at all. Therefore,
	  the const_cast should not hurt.
      */
      Iterator( const Node* p ) : n( const_cast<Node *>(p) ) {}
      //! Copy CTOR
      Iterator( const Iterator& i ) : n( i.n ) {}
      //! Assignment operator
      Iterator& operator=( const Iterator& i ){
	n = i.n;
	return *this;
      }
      //! Assignment from Position
      Iterator& operator=( const Node *p ){
	n = const_cast<Node *>(p);
	return *this;
      }

      //! Increment by one
      Iterator& operator++() { next(); return *this; }
      //! Decrement by one
      Iterator& operator--() { prev(); return *this; }
      //! Equality operator
      inline bool operator==( const Iterator& i ) const { return (n == i.n); }
      //! Inequality operator
      inline bool operator!=( const Iterator& i ) const { return (n != i.n); }
      //! Check for invalid iterator
      inline bool valid() const { return ((n != NULL) && (n->Next) && (n->Prev)); }
      //! Check for successor
      inline bool hasNext() const { return (n && (n->Next) && (n->Next->Next)); }
      //! Check for predecessor
      inline bool hasPrev() const { return (n && !n->isMarked() && (n->Prev) && (n->Prev->Prev)); }
    };
  public:
    //! The consumer iterator class working on T
    /*! \param T The class the client assumes as contained
        \param _const Create code for a constant_iterator
	\param _rev Create code for a reverse_iterator

	The original idea was to have one template and create
	the standard iterators
	\li iterator
	\li const_iterator
	\li reverse_iterator
	\li const_reverse_iterator
	
	Since C++ does not know template typedefs, we must inherit
	and redefine all CTORs and operators returning itself. For
	this reason we currently overload operator++ and operator--
	crossed for the derived classes, instead of exchanging them
	in the template immediately.

        \todo Care for tainting and the like
     */
    template<class T, bool _const = true, bool _rev = false> class _iterator : 
      public Iterator {
    public:
      typedef typename meta::IF<_const, const T&, T&>::RESULT _RetType;
      typedef typename meta::IF<_const, const T*, T*>::RESULT _RetPtr;
      //! CTOR typecast
      /*! \copydoc Iterator( const Position& p ) */
      explicit _iterator( const Node *p = NULL ) : Iterator(p) {}
      //! Copy CTOR typecast
      /*! \copydoc Iterator( const Iterator& ) */
      _iterator( const _iterator& i ) : Iterator(i) {}
      //! \overload Iterator::operator++()
      _iterator& operator++(){ 
	return static_cast<_iterator&>( Iterator::operator++() ); 
      }
      //! \overload Iterator::operator--()
      _iterator& operator--(){ 
	return static_cast<_iterator&>( Iterator::operator--() ); 
      }
      //! Return the pointer to the current node or NULL 
      _RetPtr operator->() const { 
	return static_cast<_RetPtr>( Iterator::current() ); 
      }
      //! Return the reference to the current node or crash
      _RetType operator*() const { 
	return *(static_cast<_RetPtr>( Iterator::current() )); 
      }
      //! Assignment operator
      _iterator& operator=( const _iterator& i ){
	Iterator::operator=( i );
	return *this;
      }
      //! Assignment from Position
      _iterator& operator=( const Iterator& p ){
	Iterator::operator=( p );
	return *this;
      }
      //! Insert node before current
      /*! \note since begin() starts at the first node and end()
          is behind the last, this method can insert at any place
	  in the list.
      */
      _RetPtr insert( const T& n ){
	return static_cast<_RetPtr>(Iterator::insert(&n));
      }
      //! \overload insert( const T& n )
      _RetPtr insert( const T* n ){
	return static_cast<_RetPtr>(Iterator::insert(n));
      }
      //! Remove is handy in derived classes
      T* remove() { return static_cast<T*>(Iterator::remove()); }
      //! Erase current item
      m_error_t erase() {
	return (Iterator::remove())? ERR_NO_ERROR : ERR_CANCEL;
      }
    };

    /*
     * And since C++ does not know template typedef
     * we neeed the folowing no brainers.
     *
     */

    //! The constant forward iterator
    template<class T> class const_iterator 
      : public _iterator<T,true,true> {
    public:
      typedef class _iterator<T,true,true> __iterator;
      typedef class const_iterator<T> _iter;
      //! CTOR typecast
      /*! \copydoc Iterator( const Position& p ) */
      explicit const_iterator( const Node *p = NULL ) : __iterator(p) {}
      //! Copy CTOR typecast
      /*! \copydoc Iterator( const Iterator& ) */
      const_iterator( const Iterator& i ) : Iterator(i) {}
      //! Assignment operator
      _iter& operator=( const Iterator& i ){
	Iterator::operator=( i );
	return *this;
      }
      //! \overload Iterator::operator++()
      _iter& operator++(){ 
	return static_cast<_iter&>( __iterator::operator++() ); 
      }
      //! \overload Iterator::operator--()
      _iter& operator--(){ 
	return static_cast<_iter&>( __iterator::operator--() ); 
      }
    };
    //! The non-constant forward iterator
    template<class T> class iterator 
      : public _iterator<T,false,true> {
    public:
      typedef class _iterator<T,false,true> __iterator;
      typedef class iterator<T> _iter;
      //! CTOR typecast
      /*! \copydoc Iterator( const Position& p ) */
      explicit iterator( const Node *p = NULL ) : __iterator(p) {}
      //! Copy CTOR typecast
      /*! \copydoc Iterator( const Iterator& ) */
      iterator( const Iterator& i ) : Iterator(i) {}
      //! Assignment operator
      _iter& operator=( const Iterator& i ){
	Iterator::operator=( i );
	return *this;
      }
      //! \overload Iterator::operator++()
      _iter& operator++(){ 
	return static_cast<_iter&>( __iterator::operator++() ); 
      }
      //! \overload Iterator::operator--()
      _iter& operator--(){ 
	return static_cast<_iter&>( __iterator::operator--() ); 
      }
    };
    //! The constant reverse iterator
    template<class T> class const_reverse_iterator 
      : public _iterator<T,true,false> {
    public:
      typedef class _iterator<T,true,false> __iterator;
      typedef class const_iterator<T> _iter;
      //! CTOR typecast
      /*! \copydoc Iterator( const Position& p ) */
      explicit const_reverse_iterator( const Node *p = NULL ) 
	: __iterator(p) {}
      //! Copy CTOR typecast
      /*! \copydoc Iterator( const Iterator& ) */
      const_reverse_iterator( const Iterator& i ) : Iterator(i) {}
      //! Assignment operator
      _iter& operator=( const Iterator& i ){
	Iterator::operator=( i );
	return *this;
      }
      //! \overload Iterator::operator++()
      /*! For reverse iterator, the opposite infix operator is overloaded. */
      _iter& operator++(){ 
	return static_cast<_iter&>( __iterator::operator--() ); 
      }
      //! \overload Iterator::operator--()
      /*! For reverse iterator, the opposite infix operator is overloaded. */
      _iter& operator--(){ 
	return static_cast<_iter&>( __iterator::operator++() ); 
      }
    };
    //! The non-constant reverse iterator
    template<class T> class reverse_iterator 
      : public _iterator<T,false,false> {
    public:
      typedef class _iterator<T,false,false> __iterator;
      typedef class reverse_iterator<T> _iter;
      //! CTOR typecast
      /*! \copydoc Iterator( const Position& p ) */
      explicit reverse_iterator( const Node *p = NULL ) : __iterator(p) {}
      //! Copy CTOR typecast
      /*! \copydoc Iterator( const Iterator& ) */
      reverse_iterator( const Iterator& i ) : Iterator(i) {}
      //! Assignment operator
      _iter& operator=( const Iterator& i ){
	Iterator::operator=( i );
	return *this;
      }
      //! \overload Iterator::operator++()
      /*! For reverse iterator, the opposite infix operator is overloaded. */
      _iter& operator++(){ 
	return static_cast<_iter&>( __iterator::operator--() ); 
      }
      //! \overload Iterator::operator--()
      /*! For reverse iterator, the opposite infix operator is overloaded. */
      _iter& operator--(){ 
	return static_cast<_iter&>( __iterator::operator++() ); 
      }
    };

  protected:
    /*! \class _ListAnchor
        \brief Holding the both ends of a list

	This class shall be inherited by the list
	sub-classes holding the nodes. It is
	_ListAnchor, who owns the nodes.

	\warning This class delivers _RawNode
	pointers as Node pointers. _RawNode
	does not have a vTable, so never
	use the off-end pointers as Node.
    */
    class _ListAnchor {      
    protected:
      //! The polymorphic storage for compressed Node
      union {
	//! The plain access
	struct {
	  //! Pointer to the first Node (Head.Next)
	  Node *First;
	  //! Always NULL (Head.Prev and Tail.Next)
	  Node *Zero;
	  //! Pointer to last Node (Tail.Prev)
	  Node *Last;
	} nodes;
	struct {
	  //! _RawNode part of Head
	  _RawNode _Head;
	  //! This is Last
	  Node *_Last;
	} head;
	struct {
	  //! This is First
	  Node *_First;
	  //! _RawNode part of Tail
	  _RawNode _Tail;
	} tail;
      } Anchor;
      //! Attach the end of the chain to Tail
      inline void fixTail( Node *p ){
	p->Next = static_cast<Node*>(&(Anchor.tail._Tail));
	Anchor.tail._Tail.Prev = p;
      }
      //! Initialise an empty list
      inline void init() {
	Anchor.nodes.Zero = NULL;
	Anchor.head._Head.Next = static_cast<Node*>(&(Anchor.tail._Tail));
	Anchor.tail._Tail.Prev = static_cast<Node*>(&(Anchor.head._Head));
      };
      //! Empty the list
      /*! This function deletes all nodes
          in the list and initializes the
	  list as empty.
      */      
      void clear();
      //! Deep-Copy entire list
      /*! \param l List to copy
	  \note This method is used
	  in CTORs. It does not return errors,
	  but throws. However, you must not
	  use this function with an uninitialized
	  _ListAnchor, since it will free any
	  non-empty list.
	  \note Make sure that the list does not
	  contain any nodes before calling this.
      */
      void deepCopy( const _ListAnchor& l );

    public:
      //! CTOR creates empty Anchor
      _ListAnchor() { init();}
      //! Copy CTOR copies deeply
      _ListAnchor( const _ListAnchor& l ){
	init();
	deepCopy(l);
      }
      //! Assignment operator copies deeply
      _ListAnchor& operator=( const _ListAnchor& l ){
	deepCopy(l);
	return *this;
      }
      //! DTOR deletes all entries
      ~_ListAnchor() { clear(); }
      //! Get head as _RawNode
      inline const _RawNode& Head() const { return Anchor.head._Head; };
      //! Get tail as _RawNode
      inline const _RawNode& Tail() const { return Anchor.tail._Tail; };
      //! Get head as Node* for iterator bounds
      inline const Node *pHead() const {
	return static_cast<const Node*>(&(Anchor.head._Head));
      }
      //! Get tail as Node* for iterator bounds
      inline const Node *pTail() const {
	return static_cast<const Node*>(&(Anchor.tail._Tail));
      }
      //! Check for empty list
      inline bool empty() const { return (Anchor.head._Head.Next == &(Anchor.tail._Tail)); }
    };

  public:
    //! Version information string
    static const char *VersionTag();

  };

  /*! \class DList
      \brief Double linked lists

      This class is intended for large lists or heavily and
      unnecessarily assigned lists. Copy and assigment are
      shallow. A common, reference counted list object is
      used by a whole genealogy of DList objects. DList is
      Branchable and branch() will initiate a deep-copy,
      if the list is shared.

      This class is structured as follows:
      \arg _DList The reference counted object containing the start and end 
           of the list. This object owns all Node in the list.
      \arg DList The double linked list main interface

      Nodes should be constructed like this:
      \code

      class myItem : public DList::Node {
      public:
        myItem( const myItem& );
        virtual Cloneable *clone() {
          return static_cast<Cloneable *>( new myItem( *this ) );
        }
      }
      \endcode

      The basic interfacing code finally reads about this:
      \code
      DList list;
      DList::iterator<myItem> it;
      myItem mi;
      it = list.begin();
      if(NULL == it.insert(mi)) throw( "Insertion failed!" );
      for(it=list.begin();it != list.end(); ++it) processItem(*it);
      \endcode

      iterator and const_iterator are more or less only type casting
      templates for Iterator. They should be compiled to no real
      code.

  */
  class DList : public _DListGeneric, public Branchable {
  public:
    //! The Node to inherit
    typedef _DListGeneric::Node Node;
    //! The protected Iterator is known, but not accessible by the client
    typedef _DListGeneric::Iterator Iterator;

  protected:
    //! This is the reference counted list object
    class _DList : public _DListGeneric::_ListAnchor, public RCObject {
    public:
      //! Empty CTOR
      _DList() {}
      //! Deep-Copy CTOR
      _DList( const _ListAnchor& l ) : _ListAnchor( l ) {}
      //! DTOR is virtual
      virtual ~_DList() {}
    };

    //! This is the list anchor renamed for source invariance
    typedef _DList _List;

    //! The reference counting smart-pointer to the real list
    RCPtr<_List> List;
    //! This is optimized out and establishes source invariance
    inline const _List& getList() const { return *List; }

  public:
    //! Create an empty list
    DList() : List( new _List ) {}
    //! Shallow copy
    DList( const DList& l ) : List( l.List ) {}
    //! Shallow assignment
    DList& operator=( const DList& l ) {
      List = l.List;
      return *this;
    }
    //! Deep copy if non-exclusive
    /*! \todo Check how to deal with iterators */
    virtual m_error_t branch() {
      return List.branch();
    }
    //! DTOR is virtual
    virtual ~DList() {}

    //! Overload clear to unlock the current _List and create a new one
    void clear() { List = new _List; }

    //! This kind of inheritance is not supported by C++
#include "dlist-operations.h"

  };

  /*! \class SDList
      \brief SDouble linked lists

      This class can be considered as replacement for std::list,
      however providing a distinct Node type, which can be inherited.
      Using std::list, this would only be possible for a list of
      pointers to real objects. So if you have polymorphic list
      elements and you want to avoid another layer of indirection,
      SDList is your choice.

      In contrast to DList, copy construction and assignment is
      always deep. DList and SDList are totally interface compatible.
      There are some deviations from std:list apart from not yet
      implemented features, which are necessary to support
      polymorhic list elements.

      This class is structured as follows:
      \arg _SDList The embedded object containing the start and end 
           of the list. This object owns all Node in the list.
      \arg SDList The double linked list main interface

      \note SDList::Node and DList::Node are identical being
      _DListGeneric::Node. This allows to interchange nodes
      e.g. using swap() and adds to interface compatibility.
      Most of the interface is common code held in
      dlist-operations.h, which is included into the bodies
      of both DList and SDList. Since the acces to the 
      list anchor is different in these classes, it is
      possible to write source invariant code, but it is not
      possible to solve this by inheritance or templates, without
      introducing an otherwise unnecessary indirection.

      Nodes should be constructed like this:
      \code
      class myItem : public SDList::Node {
      public:
        myItem( const myItem& );
        virtual Cloneable *clone() {
          return static_cast<Cloneable *>( new myItem( *this ) );
        }
      }
      \endcode

      The basic interfacing code finally reads about this:
      \code
      SDList list;
      SDList::iterator<myItem> it;
      myItem mi;
      it = list.begin();
      if(NULL == it.insert(mi)) throw( "Insertion failed!" );
      for(it=list.begin();it != list.end(); ++it) processItem(*it);
      \endcode

      iterator and const_iterator are more or less only type casting
      templates for Iterator. They should be compiled to no real
      code.

  */
  class SDList : public _DListGeneric {
  public:
    //! The Node to inherit from
    typedef _DListGeneric::Node Node;
    //! The protected Iterator is known, but not accessible
    typedef _DListGeneric::Iterator Iterator;

  protected:
    //! This is the embedded list object
    class _SDList : public _DListGeneric::_ListAnchor {
    public:
      //! Empty CTOR
      _SDList() {}
      //! Copy CTOR doing Deep-Copy
      _SDList( const _ListAnchor& l ) : _ListAnchor( l ) {}
      //! Assignment operator doing deep-copy
      _SDList& operator=( const _ListAnchor& l ) { 
	return static_cast<_SDList&>(_ListAnchor::operator=(l));
      }
      //! DTOR destroys all list nodes
      ~_SDList() {}
      //! Destroy all nodes
      /*! This stub function brings the protected
	  _ListAnchor::clear() to the public domain
	  of the protected nested class. This avoids
	  a friendship with SDList.
      */
      inline void clear() { _ListAnchor::clear(); }
    };

    //! This is the list anchor for source invariance
    typedef _SDList _List;

    //! The reference counting smart-pointer to the real list
    _List List;
    //! This is optimized out and establishes source invariance
    inline const _List& getList() const { return List; }
  public:
    //! Create an empty list
    SDList() {}
    //! Deep copy
    SDList( const SDList& l ) : List( l.List ) {}
    //! Deep assignment
    SDList& operator=( const SDList& l ) {
      List = l.List;
      return *this;
    }
    //! DTOR is not virtual
    ~SDList() {}

    // Dependent methods
    void clear() { List.clear(); }

    //! This kind of inheritance is not supported by C++
#include "dlist-operations.h"

  };

  /*! \class CDList
      \param T Class contained
      \param L Container Class

      This is a typecasting interface to DList or SDList. 
      It cointains no intelligence, but overloads the
      STL type functions for convenience.

      \note Apart from yet another vtable this class should
      not produce any code. However, replacing 
      \code DList tl( list ); \endcode
      with
      \code CDList<Test> tl( list ); \endcode
      adds 602 Byte to the executable (cygwin / gcc 3.4.4 ).
  */
  template<class T, class L> class CDList : public L {
  public:
    typedef typename L::template iterator<T> iterator;

    //! Default CTOR
    CDList() {}
    //! Copy CTOR
    CDList( const CDList& l ) : L( l ) {}
    //! Cast CTOR
    explicit CDList( const L& l ) : L( l ) {}
    //! Shallow assignment
    CDList& operator=( const CDList& l ){
      return static_cast<CDList>(L::operator=( l ));
    }    
    //! Shallow assignment across types
    CDList& operator=( const DList& l ){
      return static_cast<CDList>(L::operator=( l ));
    }
    //! Declare virtual DTOR
    virtual ~CDList() {}

    //! \overload DList::begin() const
    iterator begin() const {
      return iterator(L::begin());
    }
    //! \overload DList::rbegin() const
    iterator rbegin() const {
      return iterator(L::rbegin());
    }
    //! \overload DList::end() const
    iterator end() const {
      return iterator(L::end());
    }
    //! \overload DList::end() const
    iterator rend() const {
      return iterator(L::rend());
    }
    //! \overload DList::front() const
    T& front() const {
      return *(static_cast<T*>(L::front()));
    }
    //! \overload DList::back() const
    T& back() const {
      return *(static_cast<T*>(L::back()));
    }
    //! \overload DList::push_back()
    void push_back( const T& t ){
      L::push_back(&t);
    }
    //! \overload DList::push_front()
    void push_front( const T& t ){
      L::push_front(&t);
    }
    //! \overload DList::pop_back()
    void pop_back() {
      L::pop_back();
    }
    //! \overload DList::pop_front()
    void pop_front() {
      L::pop_front();
    }
  };

};}; // namespace mgr::sol

#endif // _SOL_DLIST_H_
