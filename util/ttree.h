/*! \file ttree.h
    \brief Replacement for HTree using STL and templates

    These classes also implement a hierrarchical tree
    as HTree, but make use of STL std:lists and templates.
    This means you do not inherit from TTree as from XTree,
    but you instatiate a TTree template.

    \version  $Id: ttree.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
    \author Dr. Lars Hanke
    \date 2007
*/

#ifndef _UTIL_TTREE_H_
# define _UTIL_TTREE_H_

#include <list>
#include <vector>
#include <iterator>
#include <unistd.h>
#include <RefCounter.h>
#include <mgrError.h>
#include <mgrMeta.h>

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

namespace mgr {

    /*! \class TTreeNodeBase
        \brief Base class to inherit for TTreeNodeIndirect payloads

	This base class is pure virtual and defines the
	clone() member, which copies the TTreeNodeBase
	into a new instance, i.e. performs something
	like return new( *this )

	\warning Assure that clone() is implemented in
	each leaf-class.
    */
    class TTreeNodeBase {
    public:
      virtual ~TTreeNodeBase() {}
      //! Interface definition for virtual CTOR
      virtual TTreeNodeBase *clone() const = 0;
    };

  /*! \class TTree
      \brief Hierarchical trees using STL functions and templates

      \param T Payload data to be contained
      \param indirect If true, the payload is pointed to, 
      which allows for inheritance for the price of one
      more indiretion.

      The TTree is a list of lists maintained by a current
      path, which is a vector of iterators to these lists.
      A vector of iterators is necessary, since there is
      no other canonical method to look up siblings
      of the current node.

      For navigation functions still pointers to contents
      are returned, since without the list associated with
      the iterator external functions would not be able
      to determine, whether the current iterator is valid.

      For this reason it is not as easy as with HTree to
      build node structures outside of the TTree.

      \warning This class is an experimental implementation.
      For production use HTree.

      Actually, most of TTree currently is an iterator on
      TTree. The TTree itself could be molt down to the sroot
      list. I'll play around with this case until I find
      something, which has some real advantage to HTree. If I
      never find it, I learned programming the C++ paradigma.

      \note Although this implementation of hierrachical trees
      does not include all features of HTree and although the
      test case does not instatiate all templates, the test case
      is somewhat larger: 25 kB vs. 21 kB with -O2 and without
      debugging information. The really bad thing with the template
      model, is that two instances of the TTree  eat double
      the memory. The test case with a direct and an indirect
      node type compiles to 46 kB!
  */
  template<class T, bool indirect = false>class TTree {
  protected:
    /*! \class TTreeNodeDirect
        \brief Node Template for TTree with immediate payload

	This node contains a std::list of itself and
	the payload class. The dereferencing operator*()
	returns a non-const reference to the payload.

	The node can be constructed from its payload.
	Therefore, adding to the lists can be based
	on payloads instead of the nodes itself. This
	of course copies the payloads.

	This encapsulation solves all question about who
	may own a node. The node is owned by the list, which
	is owned by another node or the TTree.       

	\warning You cannot inherit from TTreeNodeDirect. Since
	std_list.push_back() requires a copy CTOR, which
	cannot be virtual. If you must inherit, use
	TTreeNodeIndirect, i.e. set the indirect flag
	in the TTree template.

	\note TTreeNodeDirect is a protected member class, which
	will never be visible to clients of TTree.
    */
    class TTreeNodeDirect {
    protected:
      T Payload;             //!< The payload data

    public:
      //! Data type for the node lists
      typedef std::list<TTreeNodeDirect> Sequence;
      Sequence children;    //!< Sequence of children list

    public:
      //! CTOR from payload data
      TTreeNodeDirect( const T& n ) : Payload( n ) {
	xpdbg(ALLOC,"### Created TTreeNodeDirect Payload: %p from %p\n",
	      &Payload,&n);
      }
      //! copy CTOR
      TTreeNodeDirect(const TTreeNodeDirect& n) : Payload( n.Payload ) {
	xpdbg(ALLOC,"### Copied TTreeNodeDirect Payload %p to %p\n",
	      &(n.Payload), &Payload );
      }
      //! assignment operator
      TTreeNodeDirect& operator=(const TTreeNodeDirect& n){
	Payload = n.Payload;
	return *this;
      }
      //! retrieve the node contents
      T& operator*()  { return Payload; }
    };

    /*! \class TTreeNodeIndirect
        \brief Node Template for TTree with pointer to payload

	This node contains a std::list of itself and a pointer to
	the payload class. The dereferencing operator*()
	returns a non-const reference to the payload itself.

	The node can be constructed from its payload.
	Therefore, adding to the lists can be based
	on payloads instead of the nodes itself. This
	of course copies the payloads.

	This encapsulation solves all question about who
	may own a node. The node is owned by the list, which
	is owned by another node or the TTree.       

	\warning Payloads must inherit from TTreeNodeBase

	\note TTreeNodeIndirect is a protected member class, which
	will never be visible to clients of TTree.
    */
    class TTreeNodeIndirect {
    protected:
      TTreeNodeBase *Payload;  //!< Pointer to payload, heir of TTreeNodeBase

    public:
      //! Data type for the node lists
      typedef std::list<TTreeNodeIndirect> Sequence;
      Sequence children;  //!< Sequence of children list

    public:
      //! CTOR from payload data
      /*! \param n Payload data
	
          This CTOR copies the Payload using the T copy CTOR.
      */
      TTreeNodeIndirect( const T& n ) : Payload( new T( n )){
	xpdbg(ALLOC,"### Created TTreeNodeIndirect Payload: %p @ %p\n",
	      Payload,this);
      }
      TTreeNodeIndirect( const T *n ) : Payload( (n)? new T( *n ) : NULL ) {
	xpdbg(ALLOC,"### Created TTreeNodeIndirect Payload: %p -> %p @ %p\n",
	      n,Payload,this);	
      }
      //! copy CTOR
      TTreeNodeIndirect(const TTreeNodeIndirect& n) 
	: Payload( (n.Payload)? n.Payload->clone() : NULL ) {
	xpdbg(ALLOC,"### Copied TTreeNodeIndirect Payload: %p from %p @ %p\n",
	      Payload,n.Payload,this);
      }
      //! assignment operator
      TTreeNodeIndirect& operator=(const TTreeNodeIndirect& n){
	if(Payload) delete Payload;
	Payload = n.Payload->clone();
	return *this;
      }
      //! assignment from content pointer
      /*! \param p Pointer to contents - NULL accepted for empty CTOR
	
          This is a hack to avoid double allocation in adding
	  to ttrees.
      */
      TTreeNodeIndirect& operator=(const T* p){
	if(Payload) delete Payload;
	if(p) Payload = new T( *p );
	else Payload = NULL;
	xpdbg(ALLOC,"### Assigned TTreeNodeIndirect Payload: %p -> %p @ %p\n",
	      p,Payload,this);
	return *this;
      }
      //! Destructor deallocating the payload
      ~TTreeNodeIndirect() {
	xpdbg(ALLOC,"### Deleting Payload: %p for %p\n",Payload, this);
	if(Payload) delete Payload;
      }
      //! retrieve the node contents (not the pointer!)
      /*! \note This operator throws( ERR_PARAM_NULL ), if
	  the Payload is NULL.
      */
      T& operator*() { 
	if( !Payload ) mgrThrowFormat( ERR_PARAM_NULL, "Node (%p)", this );
	return static_cast<T&>(*Payload); 
      }
    };
    
    //! TTreeNode is chosen from TTreeNodeDirect or TTreeNodeIndirect according to the indirect template flag
    typedef typename meta::IF<indirect, TTreeNodeIndirect, TTreeNodeDirect>::RESULT TTreeNode;

    //! Type of the sequence list
    typedef typename TTreeNode::Sequence TTreeNodeList;
    //! Iterator on the sequence list
    typedef typename TTreeNodeList::iterator TTreeNodeIter;
    //! const Iterator on the sequence list
    typedef typename TTreeNodeList::const_iterator TTreeNodeCIter;
    //! Type of the current path
    typedef typename std::vector<TTreeNodeIter> Bookmark;

    //! \class DirectPushBackProxy
    /*! This is a branch for handling TTreeNode generation from
        content type T during initial push_back() to the Sequence.
	For the direct case no special intelligence is implemented.
    */
    class DirectPushBackProxy {
    public:
      //! push back contents to sequence
      /*! \param l Sequence list
	  \param n Contents

	  Performs the creation of the TTreeNode and copies
	  the contents. In this direct case a TTreeNode is
	  created from n for type conversion, which is then
	  copied to the Sequence, i.e. n is copied twice.
	  If copying n is large effort, use the indirect
	  switch. IndirectPushBackProxy has a method to
	  copy the payload only once.
      */
      static void push_back( TTreeNodeList& l, const T& n ){
	l.push_back(n);
      }
      //! push front contents to sequence
      /*! \sa push_back() */
      static void push_front( TTreeNodeList& l, const T& n ){
	l.push_front(n);
      }
    };

    //! \class IndirectPushBackProxy
    /*! This is a branch for handling TTreeNode generation from
        content type T during initial push_back() to the Sequence.
	For indirect nodes, at first a NULL pointer is pushed,
	which is assigned later on. While for the NULL pointer
	only a TTreeNodeIndirect is created, copied and destroyed,
	yet with the copy in the Sequence, the Payload will be copied 
	during assignment, i.e. save one copy of the Payload.
    */
    class IndirectPushBackProxy {
    public:
      //! push back contents to sequence
      /*! \param l Sequence list
	  \param n Contents

	  Performs the creation of the TTreeNode and copies
	  the contents. In this indirect case, only the
	  base node is copied twice (created and copied), while
	  the payload is assigned (copied) to the final node
	  in the list. This saves one copy of the payload.
      */
      static void push_back( TTreeNodeList& l, const T& n ){
	l.push_back( NULL );
	l.back() = &n;
      }
      //! push front contents to sequence
      /*! \sa push_back() */
      static void push_front( TTreeNodeList& l, const T& n ){
	l.push_front( NULL );
	l.front() = &n;
      }
    };

    //! The class SequencePush is used to have varying funtions in the TTreeIterator for either direct or indirect TTree.
    typedef typename meta::IF<indirect, IndirectPushBackProxy, DirectPushBackProxy>::RESULT SequencePush;

  public:
    /*! \class TTreeIterator
        \brief Iterator on a TTree

	This class just iterates the nodes. It may also
	be used to insert or remove nodes, but destroying
	the iterator will not free the nodes of the
	TTree.
	
	This is a public member class of TTree and will
	be returned to the client.
    */
    class TTreeIterator {
    protected:
      Bookmark path;
      const TTree& tree;

      //! Just a shortcut to get the pointer to payload from the list iterator
      /*! \return Pointer to payload */
      T *getCurrent() const  {
	if(path.back() == getSiblingList().end())
	  return NULL;	
	return &(**(path.back()));
      }

      //! Get list of siblings, which is located at the parent node
      const TTreeNodeList& getSiblingList() const  {
	size_t depth = path.size();
	if(depth > 1){
	  return (path[depth - 2])->children;
	} else {
	  return sroot();
	}
      }

      /*! \brief get the root list from the TTree
	  \return Reference to root list

	  This is a friend function to TTree in order
	  to access the sroot field.
      */
      const TTreeNodeList& sroot() const  {
	return tree.Root->sroot;
      }

    public:
      //! Standard CTOR
      /*! \param t Tree to iterate on

          Creates an iterator starting at the root node.

	  \note There is no empty CTOR, because the Iterator
	  must be bound to a specific tree.
      */
      TTreeIterator( const TTree& t ) : tree(t) {
	root();
      }

      //! Copy CTOR
      TTreeIterator( const TTreeIterator& i ) :
	path( i.path ), tree( i.tree ) {}

      //! Assignment
      TTreeIterator& operator=( const TTreeIterator& i ) {
	path = i.path;
	tree = i.tree;
      }	

      /*
       * Simple navigation functions
       *
       */
      //! Reset iterator to the very first node
      T *root()  {
	path.clear();
	if(tree.empty()) return NULL;
	path.push_back(const_cast<TTreeNodeList&>(sroot()).begin());
	return getCurrent();
      }

      //! Return contents of current node
      T *current() const  {
	if(path.empty()) return NULL;
	return getCurrent();
      }

      //! Move to child of current and return contents
      T *child()  {
	if(path.empty()){
	  if(tree.empty()) return NULL;
	  path.push_back(const_cast<TTreeNodeList&>(sroot()).begin());
	  return getCurrent();
	}
	if(path.back()->children.empty()) return NULL;
	path.push_back(path.back()->children.begin());
	return getCurrent();
      }
      //! Move to parent of current and return contents
      T *parent()  {
	if(path.size() < 2) return NULL;
	path.pop_back();
	return getCurrent();
      }
      //! Move to next of current and return contents
      T *next()  {
	if(!path.empty()){
	  if(path.back() == getSiblingList().end())
	    return NULL;
	  ++path.back();
	  return getCurrent();
	}
	if(tree.empty()) return NULL;
	path.push_back(const_cast<TTreeNodeList&>(sroot()).begin());
	return getCurrent();
      }
      //! Move to precessor of current and return contents
      T *previous()  {
	if(!path.empty()){
	  if(path.back() == getSiblingList().begin())
	    return NULL;
	  --path.back();
	  return getCurrent();
	}
	if(tree.empty()) return NULL;
	path.push_back(const_cast<TTreeNodeList&>(sroot()).begin());
	return getCurrent();
      }
      //! Move to first sibling and return contents
      T *first()  {
	if(path.empty()){
	  if(tree.empty()) return NULL;
	  path.push_back(const_cast<TTreeNodeList&>(sroot()).begin());
	} else {
	  path.back() = getSiblingList().begin();
	}
	return getCurrent();
      }
      //! Move to last sibling and return contents
      T *last()  {
	if(path.empty()){
	  if(tree.empty()) return NULL;
	  path.push_back(--(const_cast<TTreeNodeList&>(sroot()).end()));
	} else {
	  if(getSiblingList().empty()) mgrThrow(ERR_INT_STATE);
	  path.back() = --(getSiblingList().end());
	}
	return getCurrent();
      }

      /*
       * Node information functions
       *
       */
      //! Check if current node has children
      bool hasChildren() const  {
	if(!path.size()) return false;
	return ! (*(path.back())).children.empty();
      }
      //! Check if current node is in the root sequence
      bool hasParent() const  {
	return (path.size() > 1);
      }
      //! Check if current node has successors
      bool hasNext() const  {
	if(!path.size()) return false;
	TTreeNodeIter c = path.back();
	return (++c != getSiblingList().end());
      }
      //! Check if current node has precessors
      bool hasPrevious() const  {
	if(!path.size()) return false;
	return (path.back() != getSiblingList().begin());
      }
      //! Return the current depth in the TTree
      const size_t depth() const  {
	return path.size();
      }

      /*
       * Manipulation of the tree
       *
       */
      //! Append contents to the end of the sequnece where current is in
      /*! \param p Value to append to sequence
	  \param moveCurrent Set current iterator position to added node
          \return Error code as defined in mgrError.h
	  \todo push_back() is called with const T& as argument.
	  This causes implicit type conversion before push_back()
	  followed by copying. In the end, p is cloned twice.
	  \note For indirect nodes the SequencePush branch implements
	  an intermediate hack. push_back() creates and copies an empty
	  node. The contents are assigned and copied to the final node
	  in the list, i.e. the contents are copied once, the TTreeNode
	  twice.
      */
      m_error_t appendSequence(const T& p, bool moveCurrent = false ){
	if(!path.size()){
	  if(root()) return ERR_INT_STATE;
	  SequencePush::push_back(const_cast<TTreeNodeList&>(sroot()),p);
	  return (root())? ERR_NO_ERROR : ERR_MEM_AVAIL;
	}
	TTreeNodeList &cList = const_cast<TTreeNodeList &>(getSiblingList());
	SequencePush::push_back(cList,p);
	if(moveCurrent) path.back() = --(cList.end());
	return ERR_NO_ERROR;
      }
      //! Add contents as the first child of the current node
      m_error_t insertChild(const T& p, bool moveCurrent = false ){
	if(!path.size()) return appendSequence(p, moveCurrent);

	TTreeNodeList &cList = *(path.back()).children;
	SequencePush::push_front(cList,p);
	if(moveCurrent) path.push_back(cList.begin());
	return ERR_NO_ERROR;
      }
      //! Append contents to the child sequence of the current node
      m_error_t appendChild(const T& p, bool moveCurrent = false ){
	if(!path.size()) return appendSequence(p, moveCurrent);

	TTreeNodeList &cList = (*(path.back())).children;
	SequencePush::push_back(cList,p);
	if(moveCurrent) path.push_back(--(cList.end()));
	return ERR_NO_ERROR;
      }

      /*
       * Iteration function
       *
       */
      //! Move to child sequence or next in sequence
      T *iterate( size_t *dpth = NULL )  {
	if(hasChildren()){
	  T *s = child();
	  if(dpth) *dpth = depth();
	  return s;
	} 
	if(hasNext()) {
	  T *s = next();
	  if(dpth) *dpth = depth();
	  return s;
	}
	// Neither has siblings nor children
	T *s;
	do {
	  // pdbg("### Resume: %p (%d)\n", &(*(path.back())), path.size());
	  s = parent();
	  if(!s) break;
	  // pdbg("### Resume parent: %p (%d)\n", &(*(path.back())), path.size());
	  s = next();
	} while(!s);
	if(dpth) *dpth = depth();
	return s;
      }
      
    };

  private:
    /*! \class RootList
        \brief This class holds the actual tree as reference counted object

	It is referenced by TTree::Root, which is a template smart
	pointer implementing all the bookkeeping automagically
	during construction and destruction of TTree.
    */
    class RootList : public RCObject {
    protected:
      //! Deep-Copy a TTree, sequence per sequence
      /*! \param dest Destination of copy
          \param source list

	  After copy dest will have identical contents to
	  src, but will be a physically separated TTreeNodeList.

	  \note I considered to copy the list using the inbuilt
	  copy CTOR, but since the copy CTOR of TTreeNode deliberately
	  does only copy the payload and not the children list, we have to
	  walk the list anyway.
      */
      static void copySequence( TTreeNodeList& dest, const TTreeNodeList& src )   {
	/*! \note clear() destroys the entire list. \sa ~RootList() */
	if(!dest.empty()) dest.clear();
	for( TTreeNodeCIter i = src.begin();
	     i != src.end();
	     ++i) {
	  dest.push_back(*i);
	  if( !(*i).children.empty() )
	    copySequence( dest.back().children, (*i).children  );
	}
      }

    public:
      //! The real data of the root sequence
      TTreeNodeList sroot;
      //! Empty CTOR creates empty list by std::list CTOR
      RootList() {
	xpdbg(ALLOC,"### Empty create TTree %p\n",&sroot);
      }      
      //! Copy CTOR must deep-copy
      /*! \param l Tree structure to copy

          The copy CTOR is invoked from RCPtr<RootList>::init(), i.e.
	  from the Initialised CTOR, the copy CTOR, and assignment.
	  If the target is not sharable, it is deep-copied using
	  this function.
      */
      RootList( const RootList& l ){
	xpdbg(ALLOC,"### Deep copy TTree %p -> %p\n",&(l.sroot),&sroot);
	copySequence( sroot, l.sroot );
      }
      //! DTOR does nothing
      /*! The DTOR all sub-lists, because it destroys
	  the nodes holding the list objects. Destroying the child
	  list objects, will in turn destroy all child nodes and
	  their lists of children.
      */
      ~RootList() {
	xpdbg(ALLOC,"### Destroy TTree %p\n", &sroot);
      }      
    };

  protected:
    /*! \brief Smart-Pointer to the root list

        This is a smart pointer doing all the bookkeeping with RCObject.
	In particular, if releases the object, when running out of scope.
	RCObject decides based on the counter, whether to destroy
	itself or not.
    */
    RCPtr<RootList> Root;
    //! Allow the Iterator to get the root list
    friend const TTreeNodeList& TTreeIterator::sroot() const;
    //! Get the root list from the reference counted location
    const TTreeNodeList& sroot() const  {
      return Root->sroot;
    }      

  public:
    //! CTOR
    TTree() : Root( new RootList ) {}
    //! Copy CTOR
    TTree(TTree& t): Root( t.Root ) {}
    //! Assignment
    /*! \note Assignment just passes a reference, no deep copy
        is performed unless the tree is tainted. By the time being,
	the tree will never be tainted.
    */
    TTree& operator=( const TTree& t ){
      Root = t.Root;
    }
    //! DTOR
    virtual ~TTree() {};

    //! Deep copy a tree to this one
    /*! \param t TTree to deep-copy
        \return Error code as defined in mgrError.h

	\todo This is a classic place to put try{}catch()
	and translate to error codes
    */
    m_error_t copy( const TTree& t )  {
      Root = new RootList( *(t.Root) );
      //if( !Root ) return ERR_CLS_CREATE;
      return ERR_NO_ERROR;
    }
      
    //! Report whether the tree is empty
    bool empty() const  {
      return sroot().empty();
    }
  };
};

#ifndef _UTIL_DEBUG_H_
// Decontaminate namespace
# undef pdbg
# undef xpdbg
#endif

#endif // _UTIL_TTREE_H_
