/*
 *
 * Hierarchical Trees
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: htree.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  HTreeNode - simple skeleton node for HTrees
 *  HTree     - actually an iterator referencing a root HTreeNode
 *
 * This defines the values:
 *
 */

/*! \file htree.h
    \brief Hierarchical trees

    Hierarchical trees is a generic data structure of any kind of
    directories or TLV structures. It fits well the needs of LDAP,
    ASN.1 or XML.

    The data model is a sequence of nodes, which may have a sequence of 
    child nodes each, recursively.

    The class HTree contains the engine operating on HTreeNode
    entries. This already implements the entire data structure. The classes
    shall be used by inheriting a custom node from HTreeNode and
    use or inherit from XTree with the descendent node as template argument.
    XTree will be correctly typed for convenient use. 

    \author Dr. Lars Hanke
    \date 2006-2007
*/

#ifndef _UTIL_HTREE_H_
# define _UTIL_HTREE_H_

#include <unistd.h>
#include <mgrError.h>
#include <vector>

namespace mgr {

  /*! \class HTreeNode
      \brief Skeleton node for HTree

      The minimal node skeleton for a system of singly linked
      lists. Conatins a pointer to next in sequence and
      to the head of a list of children.

      When implementing data as HTree inherit from HTreeNode
      for your data items. HTreeNode has no virtual DTOR in
      order to spare a vtable for HTreeNodes. The node type
      specific routines, especially for deep copy and deallocation
      are put to HTree: HTree::copyNode(), HTree::freeNode()

      \todo Currently we have no assignment operator, but this
      should also go to HTree.
  */
class HTreeNode {
  //! HTree needs to modify next and child, therefore it's a friend
 friend class HTree;

 protected:
  class HTreeNode *next;   //!< next node in sequence (sibling)
  class HTreeNode *child;  //!< first node of children sequence (child)
  //! There is no sensible copy CTOR
  HTreeNode( const HTreeNode& ) : next( NULL ), child( NULL ) {}
  //! There is no sensible assignment operator
  HTreeNode& operator=( const HTreeNode& ) { return *this; }

 public:
  //! Empty CTOR
  HTreeNode() : next( NULL ), child( NULL ) {}
  //! Default DTOR
  ~HTreeNode(){};

  //! Canonical method to get next sibling pointer
  inline HTreeNode *getNext(void) const { return next; }

  //! Canonical method to get children sequence pointer
  inline HTreeNode *getChild(void) const { return child; }
};

  /*! \class HTree
      \brief Skeleton maintenance structure and methods

      This class implements all navigation and editing 
      of HTreeNode structures. It maintains a current path
      for use as a directory. 

      Since HTreeNode has no contents, HTree implements 
      all deep copy related methods operating on heirs
      of HTreeNode. This spares HTreeNode heirs a vtable, but
      still allows for deep copy and automatical deallocation
      when inheriting from HTree and overloading these
      methods as done in XTree.

      \todo Some method to insert a root node, which has the
      current tree as children.
  */
class HTree {
 public:
  //! This is a path in a HTree
  /*! The idea of a path is similar to directories
      starting from a root node through subdirectories
      to the final leaf. This kind of path is stored
      in a Bookmark.
  */
  typedef std::vector <HTreeNode *> Bookmark;

 protected:
  Bookmark path;         //!< The current path in the HTree
  HTreeNode *sroot;      //!< Pointer to the root node

  /*! \brief Central initialization routine for CTORs
      \param root Root node of the tree

      Sets up all protected properties to initialize with
      a tree rooted in root. The current item is set to
      root. If root is NULL an empty tree is set up.
  */
  void initTree(HTreeNode *root);

  //! Insert a child node to the beginning of the children sequence
  /*! \param parent Parent node to add child
      \param node Node to add as first child

      If the child shall be inserted at another place of the
      children sequence, follow HTreeNode::child and use
      insertNext(). This does not affect the current item
      of the HTree.
  */
  void insertChild(HTreeNode *parent, HTreeNode *node) const;

  //! Insert a note into a sequence
  /*! \param precessor Node to add node behind
      \param node Node to add behind precessor

      This function does not affect the current item
      of the HTree. To insert a new root node, insert
      the current root after the new node and the the
      new node as sroot.
  */
  void insertNext(HTreeNode *precessor, HTreeNode *node) const;

  //! Set child entry of node to empty (NULL)
  inline void clearChild(HTreeNode *node) const {
    node->child = NULL;
  }

 public:
  //! Create an empty HTree
  /*! \todo path.clear() is superfluous, isn't it */
  HTree() : sroot( NULL ) {
    path.clear();
  }

  //! Create a HTree from linked HTreeNode
  /*! \param n Root node of initialisation tree

      This function hooks the current node as root node
      to sroot and initializes the path to this root node.
      There are no sanity checks performed on the
      tree linkage and the node structure is not copied.
      Both is impossible, since the real type of the nodes
      is unknwon.
  */
  HTree(HTreeNode& n){
      initTree(&n);      
  }

  //! \copydoc HTree(HTreeNode& n)
  HTree(HTreeNode* n){
      initTree(n);
  }

  //! copy CTOR
  /*! \param t HTree to copy

      Only maintenance information is copied, the node structure is
      not deep copied.
  */
  HTree(const HTree& t){
    sroot = t.sroot;
    path = t.path;
  }

  //! Assignment operator 
  /*! \param t HTree to copy

      Only maintenance information is copied, the node structure is
      not deep copied.
  */
  inline HTree& operator=(const HTree& t){
    sroot = t.sroot;
    path = t.path;
    return *this;
  }    

  //! Default DTOR
  virtual ~HTree(){};
  
  //! Unhook the node structure from the HTree
  /*! This method does not do any deallocation. Just
      the maintenance information is reset to an empty tree.

      If your implementation of HTree has a notion of owning
      nodes, re-implement clear() to free the associated
      memory space. If you own all of them, this can be
      achieved by remove(sroot, true) before calling clear().
  */
  virtual void clear(void){
    path.clear();
    sroot = NULL;
  }

  //! Check for empty tree
  /*! \return true, if the tree is empty */
  inline bool isEmpty(void){
    return (sroot == NULL);
  }

  //! Deep copy a HTree
  m_error_t clone(const HTree& t);

  //! Get leaf node from the current path
  /*! \return current leaf node or NULL if path is empty 
      \note It is possible to have an empty path in a non-empty
      tree. This is just that no selection has been done. You might
      restart using root().
   */
  inline HTreeNode *current( void ) const{
    if (path.empty()) return NULL;
    return path.back();
  }

  //! Move upwards in current path
  /*! \return parent node or NULL if no more parent exists

      Returns the parent node and makes it the current
      node.

      \note The parent of a top level sequence node is NULL
      and the path will be set to undefined, i.e. current()
      will be NULL.
  */
  inline HTreeNode *parent(void){
    path.pop_back();
    // if(path.empty()) return NULL;
    // This is done by current() already
    return current();    
  }
  
  //! Move downwards in current path
  /*! \return first child node or NULL if the current node is leaf

      Returns the child node and makes it the current
      node. If no child node exists, the path will not be changed.
  */
  inline HTreeNode *child(void){
    HTreeNode *t;
    if((t=path.back()->child)){
      path.push_back( t );
      return t;
    }
    return NULL;
  }

  //! Move sidewards in current path
  /*! \return next node in sequence from the current or NULL if the sequence is finished

      Returns the next node in the sequence and makes it the current
      node. If no follower node exists, the path will not be changed.
  */
  inline HTreeNode *next(void){
    if(!(path.back()->next))
      return NULL;
    path.back() = path.back()->next;

    return current();
  }

  //! Move current node to root node of the HTree
  /*! \return pointer to root node or NULL if the tree is empty. */
  inline HTreeNode *root(void){
    path.clear();
    if(sroot) path.push_back(sroot);

    return sroot;
  }

  //! Check if a node is a child of another
  /*! \param n node to check for being the child
      \param p potential parent node; is set as current() if NULL is passed
      \return true if n is in the child sequence of p
  */
  bool isChildOf(const HTreeNode *n, const HTreeNode *p = NULL ) const {
    if(!p) p = current();
    if(!p || !n ) return false;
    p = p->child;
    while(p && (p != n)) p = p->next;
    return ( p == n );
  }
  
  //! Copy current path to external
  /*! \return Copy of the current path as Bookmark type

      Shall be used with bookmark( const Bookmark& p )
      to save and restore paths.
  */
  Bookmark bookmark(void) const {
    return path;
  }

  //! Check whether a path is valid inside the current HTree
  /*! \param p A stored path as returned from bookmark()
      \return Size of the path, or negative valid depth
  */
  ssize_t pathValidDepth( const Bookmark& p ) const;

  //! Restore a save path to current
  /*! \param p A stored path as returned from bookmark()
      \return error code as defined in mgrError.h

      If the path does not match the node structure of
      the HTree the function returns ERR_PARAM_RANG.
      Otherwise the path is copied to the current path
      and no error is returned.
  */
  m_error_t bookmark( const Bookmark& p ) {
    if( pathValidDepth( p ) <= 0 ) return ERR_PARAM_RANG;
    path = p;
    return ERR_NO_ERROR;
  }

  /*! \brief Insert node at the head of the child sequence of current node
      \param c node to insert
      \param moveCurrent If true, set inserted node as current()

      This method inserts a node / subtree as the first child
      of the current() node. The current node is only changed
      to the new node, if explicitly requested by moveCurrent.

      \note The current node is determined by force_current(), i.e.
      undefined paths will operate using the root node.
  */
  HTreeNode *insertChild(HTreeNode *c, bool moveCurrent = false);

  /*! \brief Insert node immediately behind current node into sequence
      \param c node to insert
      \param moveCurrent If true, set inserted node as current()

      This method inserts a node / subtree as the 
      immediate follower
      of the current() node. The current node is only changed
      to the new node, if explicitly requested by moveCurrent.

      \note The current node is determined by force_current(), i.e.
      undefined paths will operate using the root node.
  */
  HTreeNode *insertNext(HTreeNode *c, bool moveCurrent = false);

  /*! \brief Insert node as final node in the sequence of children of 
      the current node
      \param c node to insert
      \param moveCurrent If true, set inserted node as current()

      This method inserts a node / subtree as the 
      the final node in the sequence of children
      of the current() node. The current node is only changed
      to the new node, if explicitly requested by moveCurrent.

      \note The current node is determined by force_current(), i.e.
      undefined paths will operate using the root node.
  */
  HTreeNode *appendChild(HTreeNode *c, bool moveCurrent = false);

  /*! \brief Insert node as final node in the sequence of 
      the current node
      \param c node to insert
      \param moveCurrent If true, set inserted node as current()

      This method inserts a node / subtree as the 
      the final node in the sequence
      of the current() node. The current node is only changed
      to the new node, if explicitly requested by moveCurrent.

      \note The current node is determined by force_current(), i.e.
      undefined paths will operate using the root node.
  */
  HTreeNode *appendNext(HTreeNode *c, bool moveCurrent = false);
  
  //! Non-recursive iterator
  /*! \retval depth the current depth of the path
      \return next node in the HTree or NULL

      Usage for browsing the entire tree is resetting
      the current path by root() and calling
      iterate() until it returns NULL. The tree is iterated
      as children before next in sequence.
      
      \todo For XML and related trees, we need a callback
      when leaving a scope.
  */
  class HTreeNode *iterate(int *depth = NULL);
  
  //! Return the depth of the current path
  inline int depth(void) const {
    return path.size();
  }
  
  //! Get the last node in the current sequence
  inline HTreeNode *lastSibling(void){
      HTreeNode *p=current();
      for(;p->next;p=p->next);
      path.back() = p;
      return p;
  }
  
  //! Get the first node in the current sequence
  HTreeNode *firstSibling(void);

  //! Remove current node from HTree
  HTreeNode *slice(void);

  //! Deallocate memory of a node
  /*! \param n Node to deallocate

      The function deallocates the node. It explicitly does not unlink
      the node or frees subnodes. This shall be done before calling
      freeNode(). Overload this in your heir of HTree to match 
      the requirements of your node type.
  */
  virtual void freeNode(HTreeNode *n) const { delete n; }

  //! Deep copy a node
  /*! \return Pointer to a new node
    
      The trivial implementation on HTreeNode simply allocates
      a new empty node. If any contents are involved overload
      this function to allocate the correct type and copy the
      contents. The node linkage shall not be copied, since the
      node will be inserted into a new tree.

      static_cast to HTreeNode when overloading!
  */
  virtual HTreeNode *copyNode(const HTreeNode *) const { 
    //xpdbg(MARK,"### HTree::copyNode() CTOR\n");
    // There is nothing to copy for a copy CTOR of HTreeNode
    return new HTreeNode; 
  }

  //! Unlink and deallocate node
  /*! \param c node to remove
      \param rfree Deallocate node sequence if true

      The function unlinks the entire sequence of nodes starting
      at c. If rfree is true, this node and all successors will be
      deallocated. In any case all children of all nodes of the
      sequence will be unlinked and deallocated recursively.
  */
  void remove(HTreeNode *c, bool rfree = false) const;

  //! Unlink and deallocate current node
  /*! \param free Deallocate node if true

      This is a wrapper for remove(HTreeNode *, bool), which
      performs a slice() to cut the current node out of
      the HTree. It will deallocate the current node with all
      children, but will not follow up other nodes in the same
      sequence in the HTree.
  */
  inline void remove(bool free = false ){
      HTreeNode *n = slice();
      remove(n, free);
  }

  //! Deep copy a node with subnodes
  /*! \param n Root node of subtree to copy
      \return Copy of n with the entire sequence and children also copied and linked

      This function implements the HTree topology on 
      copyNode() to copy entire subtrees.
  */
  HTreeNode *copy(HTreeNode *n = NULL) const;

};

  /*! \class XTree

      This is a HTree with node type T, which must of 
      course inherit HTreeNode. It's merely a 
      static_cast interface translator and implements 
      node specific deletion and copy referring to the
      node CTOR and DTOR.
  */
template <class T> class XTree : public HTree {
protected:
  inline void insertChild(T *parent, T *node) const {
    HTree::insertChild(parent,node);
  }
  inline void insertNext(T *precessor, T *node) const {
    HTree::insertNext(precessor,node);
  }
  
public:
  XTree() : HTree() {};
  XTree(T& n) : HTree(&n) {};
  XTree(T* n) : HTree(n) {};    
  XTree(const XTree& t) : HTree(t) {};
  inline XTree<T>& operator=(const XTree<T>& t){
    HTree::operator=(t);
    return *this;
  }

  virtual ~XTree() {}

  inline m_error_t clone(const XTree<T>& t){
    return HTree::clone(t);
  }

  inline T *current(void) const { return static_cast<T *>(HTree::current()); }
  inline T *parent(void){ return static_cast<T *>(HTree::parent());  }
  inline T *child(void){ return static_cast<T *>(HTree::child());  }
  inline T *next(void){ return static_cast<T *>(HTree::next());  }
  inline T *root(void){ return static_cast<T *>(HTree::root());  }
  inline T *insertChild(T *c, bool moveCurrent = false){ 
    return static_cast<T *>(HTree::insertChild(c,moveCurrent));  
  }
  inline T *insertNext(T *c, bool moveCurrent = false){ 
    return static_cast<T *>(HTree::insertNext(c,moveCurrent));  
  }
  inline T *appendChild(T *c, bool moveCurrent = false){ 
    return static_cast<T *>(HTree::appendChild(c,moveCurrent));  
  }
  inline T *appendNext(T *c, bool moveCurrent = false){ 
    return static_cast<T *>(HTree::appendNext(c,moveCurrent));  
  }
  inline T *iterate(int *depth = NULL){ 
    return static_cast<T *>(HTree::iterate(depth)); 
  }
  
  // inline int depth(void) const - inherited
  
  inline T *lastSibling(void){ 
    return static_cast<T *>(HTree::lastSibling()); 
  }  
  inline T *firstSibling(void){ 
    return static_cast<T *>(HTree::firstSibling()); 
  }
  inline T *slice(void){ 
    return static_cast<T *>(HTree::slice()); 
  }

  inline T*copy(const T* t = NULL){
    return static_cast<T*>(HTree::copy(t));
  }
  
  /*! \brief Node DTOR
      \param n Node to delete

      The virtual node DTOR in the XTree instead of the node T 
      saves memory by not having the VTable with each node. The default
      implementation here should call the correct DTOR and
      should be sufficient for most purposes.

      The node is not expected to free any subsequent nodes. It shall just
      delete itself.
  */
  virtual void freeNode(HTreeNode *n) const { delete static_cast<T *>(n); }

  /*! \brief Node copy CTOR
      \param n Node to copy
      \return Copy of n as created by the copy CTOR of node type T

      The virtual copy CTOR in the XTree instead of the node T 
      saves memory by not having the VTable with each node. The default
      implementation here should call the correct copy CTOR and
      should be sufficient for most purposes.
  */
  virtual HTreeNode *copyNode(const HTreeNode *n) const { 
    //xpdbg(MARK,"### XTree::copyNode()\n"); 
    return static_cast<HTreeNode *>(new T(*(static_cast<const T*>(n)))); 
  }
};

}; // namespace mgr
#endif // _UTIL_HTREE_H_
