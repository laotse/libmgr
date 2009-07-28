/*! \file HTree.h
    \brief Replacement for mgr::HTree using sol concepts

    These classes also implement a hierrarchical tree
    as HTree, but make use of STL std:lists and templates.
    This means you do not inherit from HTree as from XTree,
    but you instatiate a HTree template.

    \version  $Id: htree-sol.h,v 1.5 2008-05-15 20:58:24 mgr Exp $
    \author Dr. Lars Hanke
    \date 2007
*/

#ifndef _SOL_HTREE_H_
# define _SOL_HTREE_H_

#include <vector>
#include <unistd.h>
#include <dlist.h>

#if defined(_UTIL_DEBUG_H_) && defined(DEBUG)
// we have included the debugger and we have DEBUG set
# if !defined(DEBUG_ALLOC)
//! No allocation debugging
/*! We have some debug information in ttree.h depending on 
    DEBUG_ALLOC. Define it as false.
*/
#  define DEBUG_ALLOC 0
# endif
#endif
#ifndef _UTIL_DEBUG_H_
// we have not included the debugger
//! Define debug output as void
# define pdbg(...) while(0)
//! Define debug output as void
# define xpdbg(...) while(0)
#endif

namespace mgr { namespace sol {

  class _HTreeGeneric {
  protected:
    class Iterator;
    class _TreeAnchor;
  public:
    //! The generic node to inherit from
    class Node : public _DListGeneric::Node {
      friend class Iterator;
      friend class _TreeAnchor;
    private:
      //! The list of all children
      SDList Children;
    protected:
      void cloneChildren( const Node& l ){
	SDList::const_iterator<Node> n;
	for(n = l.Children.begin();
	    n != l.Children.end();
	    ++n){
	  // push_back() clone()'s, unless heap-marked
	  Children.push_back( n.operator->() );
	  if( !(n->Children.empty())){
	    static_cast<Node*>(Children.back())->cloneChildren( *n );
	  }
	}	
      }
    public:
      //! CTOR
      /*! All construction is done in the
	  base class and in SDList.
      */
      Node() {}
      //! Copy CTOR does not copy linkage
      Node( const Node& ) {}
      //! Assignment is done for contents, not for linkage
      Node& operator=( const Node& ) { return *this; }
      //! DTOR is virtual
      /*! The DTOR of the list, will reap all 
          children recursively, since reaping
	  the child, will destroy its list.
      */
      virtual ~Node() {}
      //! You must inherit
      virtual Cloneable *clone() const = 0;

      //! check for children
      inline bool hasChildren() const { return !Children.empty(); }
    
    };
    //! How to navidage Children lists
    typedef SDList::iterator<Node> _iterInt;
    //! The list of parents of the current Node
    typedef std::vector< _iterInt > Path;
  protected:
    class Iterator {
    protected:
      /*! \brief The current path

	  Path[0] is the pseudo-node containing
          the root sequence. It cannot be accessed.
      */
      Path path;
    public:
      Iterator( const Iterator& i ) : path( i.path ) {}
      explicit Iterator( const Node *n = NULL ) {
	if( n ){
	  // _DListGeneric::Iterator has implicit CTOR for const _DListGeneric::Node *
	  _iterInt i( n );
	  path.push_back( i );
	}
      }
      Iterator& operator=( const Iterator& i ){
	path = i.path;
	return *this;
      }
      Iterator& operator=( const Node& n ){
	path.clear();
	// _DListGeneric::Iterator has implicit CTOR for const Node *
	_iterInt i( &n );
	path.push_back( i );
	return *this;
      }
      operator bool() const { 
	return !operator!();
      }
      bool operator!() const {
	if( path.size() < 2 ) return true;
	return !(path.back().valid());
      }
      Node *operator->() const { return static_cast<Node*>(path.back().operator->()); }
      Node &operator*() const { return *(static_cast<Node*>(path.back().operator->())); }
      Iterator& operator++(){
	++(path.back());
	return *this;
      }
      Iterator& operator--(){
	--(path.back());
	return *this;
      }
      //! Get begin of current sequence
      const Node& begin() const;
      //! Get off-end of current sequence
      const Node& end() const;
      //! Move deeper into tree
      Iterator& child();
      //! Move upwards in tree
      Iterator& parent();
      //! Reset to root node
      Iterator& root();
      /*
       * Node information functions
       *
       */
      //! Check if current node has children
      /*! \note Root node may have children, 
	  but its children have no parent!
      */
      bool hasChildren() const  {
	if(!path.size()) return false;
	return path.back()->hasChildren();
      }
      //! Check if current node is in the root sequence
      bool hasParent() const  {
	return (path.size() > 2);
      }
      //! Check if current node has successors
      bool hasNext() const  {
	if(!(*this)) return false;	
	return path.back().hasNext();
      }
      //! Check if current node has precessors
      bool hasPrev() const  {
	if(!(*this)) return false;
	return path.back().hasPrev();
      }
      //! Return the current depth in the HTree
      const size_t depth() const  {
	if(path.size() < 2) return 0;
	return path.size() - 2;
      }

      /*
       * Iteration function
       *
       */
      //! Move to child sequence or next in sequence
      Node *iterate( size_t *dpth = NULL );
    protected:
      Node *current() const { return static_cast<Node*>(path.back().operator->()); }
      /*! \param j Node to insert
	  \return New node or NULL, if Iterator is invalid
      */
      Node *insert( const Node *j ) {	
	if( !(*this) ) return NULL;
	return static_cast<Node*>(path.back().insert( j ));
      }
      /*! \return Pointer to removed Node or NULL
	
      After removing the Node the Iterator
      is advanced to the next node towards
      the end of the DList.
      
      The Node removed is unlinked and
      heapMark()'ed, but not deleted.
      */
      Node *remove() {	
	if( !(*this) ) return NULL;
	return static_cast<Node*>(path.back().remove());
      }
      /*! \param j Node to insert
	  \return New node or NULL, if Iterator is invalid

	  This function inserts the node as the first
	  node in the children sequence of the
	  current Iterator.

	  \note insertChild is available on empty, but
	  still initialized, Iterator.
      */
      Node *insertChild( const Node *j ){
	if( !(path.size()) ) return NULL;
	const Node &_n = *(path.back());
	_iterInt n( _n.Children.begin() );
	return static_cast<Node*>(n.insert( j ));
      }
    };
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
      /*! \copydoc Iterator( const Node *n ) */
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
      _iterator& child(){
	return static_cast<_iterator&>( Iterator::child() ); 
      }
      _iterator& parent(){
	return static_cast<_iterator&>( Iterator::parent() ); 
      }
      _iterator& root(){
	return static_cast<_iterator&>( Iterator::root() ); 
      }
      //! Return the pointer to the current node or NULL 
      _RetPtr operator->(){ 
	return static_cast<_RetPtr>( Iterator::current() ); 
      }
      //! Return the reference to the current node or crash
      _RetType operator*(){ 
	return *(static_cast<_RetPtr>( Iterator::current() )); 
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
      _RetPtr insertChild( const T& n ){
	return static_cast<_RetPtr>(Iterator::insertChild(&n));
      }
      _RetPtr insertChild( const T* n ){
	return static_cast<_RetPtr>(Iterator::insertChild(n));
      }
      // begin() and end() return Node& and not T&! - inherit
      _RetPtr iterate( size_t *d = NULL ){
	return static_cast<_RetPtr>(Iterator::iterate(d));
      }
    };
    class _TreeAnchor : public Node {
    public:
      _TreeAnchor() {}
      _TreeAnchor( const _TreeAnchor& a ) {
	Children = a.Children;
      }
      //! DTOR is virtual
      /*! Destroying _TreeAnchor, will also destroy
	  the inherited Node, which in turn destroys
	  the SDList member, causing the entire tree
	  to be physically destroyed. This is why
	  _TreeAnchor owns the nodes.
      */
      virtual ~_TreeAnchor() {}
      //! You cannot clone the anchor
      virtual Cloneable *clone() const { 
	mgrThrowExplain( ERR_INT_STATE, "cloned sol::HTree anchor" );
      }
      //! The tree is empty, if the anchor has no children
      inline bool empty() const {
	return !hasChildren();
      }
      //! Get the root node to initialize Iterator
      const Node& root() const {
	return *(static_cast<const Node*>(this));
      }
    };
  public:
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
      //! Assignment operator
      _iter& operator=( const Node& i ){
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
      _iter& child() {
	return static_cast<_iter&>( __iterator::child() ); 
      }
      _iter& parent() {
	return static_cast<_iter&>( __iterator::parent() ); 
      }
      _iter& root() {
	return static_cast<_iter&>( __iterator::root() ); 
      }
    };

    static const char *VersionTag();
  };

  class HTree : public _HTreeGeneric, public Branchable {
  protected:
    class _HTree : public _HTreeGeneric::_TreeAnchor, public RCObject {
    public:
      _HTree() {}
      _HTree( const _TreeAnchor& t ) : _TreeAnchor( t ) {}
      virtual ~_HTree() {}
    };
    RCPtr< _HTree > Anchor;
    inline const _HTree& getAnchor() const { return *Anchor; }

  public:
    HTree() : Anchor( new _HTree ) {}
    HTree( const HTree& t ) : Anchor( t.Anchor ) {}
    HTree& operator=( const HTree& t ) {
      Anchor = t.Anchor;
      return *this;
    }
    virtual m_error_t branch() { return Anchor.branch(); }
    virtual ~HTree() {}
    void clear() { Anchor = new _HTree; }
    const Node& root() const { return static_cast<const Node&>(getAnchor()); }
    bool empty() const { return getAnchor().empty(); }
  };

};}; // namespace mgr::sol

#ifndef _UTIL_DEBUG_H_
// Decontaminate namespace
# undef pdbg
# undef xpdbg
#endif

#endif // _SOL_HTREE_H_
