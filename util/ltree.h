/*! \file ltree.h
    \brief Replacement for HTree using STL

    These classes also implement a hierrarchical tree
    as HTree, but make use of STL std:lists.
*/

#ifndef _UTIL_LTREE_H_
# define _UTIL_LTREE_H_

#include <list>
#include <vector>
#include <unistd.h>
#include <mgrError.h>

namespace mgr {

  /*! \class LTreeNode
      \brief Inheritable node for LTree

      This node contains a std::list with pointers
      to its children. This level of indirection
      was necessary to implement LTreeNode as a
      class for inheritance instead of a template.

      The idea to avoid templates had been to allow
      the construction of node structures outside
      of the LTree and to make the LTree itself a
      class instead of a template. However, in order to
      really produce something different from HTree
      LTree should be a template.

      Making LTreeNode a template with the payload
      as a member, will invoke the CTOR for copy.
      This makes a couple of things bullet proof,
      but also requires careful design of CTOR and
      assignment operators in order to leverage
      the benefits of _wtBuffer and the like.
  */
  class LTreeNode {
    friend class LTree;
  public:
    typedef std::list<LTreeNode *> Sequence;
  protected:
    Sequence children;
  };

  /*! \class LTree
      \brief Hierarchical trees using STL functions

      The LTree is a list of lists maintained by a current
      path, which is a vector of iterators to these lists.
      A vector of iterators is necessary, since there is
      no other canonical method to look up siblings
      of the current node.

      For navigation functions still pointers to elements
      are returned, since without the list associated with
      the iterator external functions would not be able
      to determine, whether the current iterator is valid.

      For this reason it is not as easy as with HTree to
      build node structures outside of the LTree.
  */
  class LTree {
  public:
    typedef LTreeNode::Sequence::iterator LTreeNodeIter;
    typedef std::vector<LTreeNodeIter> Bookmark;
  protected:
    LTreeNode::Sequence sroot;
    Bookmark path;

    LTreeNode::Sequence& getSiblingList() const {
      size_t depth = path.size();
      if(depth > 1){
	return (path[depth - 2])->children;
      } else {
	return sroot;
      }
    }

  public:
    // CTOR and DTOR
    LTree() {}
    LTree(LTreeNode *n){
      sroot.push_back(n);
      path.push_back(sroot.begin());
    }
    LTree(LTreeNode::Sequence& l): sroot(l){
      path.push_back(sroot.begin());
    }
    virtual ~LTree() {};

    // simple navigation
    LTreeNode *current() const {
      if(path.empty()) return NULL;
      return *(path.back());
    }
    LTreeNode *child() {
      if(path.empty()){
	if(sroot.empty()) return NULL;
	path.push_back(sroot.begin());
	return *(path.back());
      }
      if(path.back()->children.empty()) return NULL;
      path.push_back(path.back()->children.begin());
      return *(path.back());
    }
    LTreeNode *parent(){
      if(path.empty()) return NULL;
      path.pop_back();
      if(path.empty()) return NULL;
      return *(path.back());
    }
    LTreeNode *root() {
      path.clear();
      if(sroot.empty()) return NULL;
      path.push_back(sroot.begin());
      return *(path.back());
    }
    LTreeNode *next() {
      if(!path.empty()){
	if(path.back() == getSiblingList().end())
	  return NULL;
	return *(++(path.back()));
      }
      if(sroot.empty()) return NULL;
      path.push_back(sroot.begin());
      return *(path.back());
    }
    LTreeNode *previous() {
      if(!path.empty()){
	if(path.back() == getSiblingList().begin())
	  return NULL;
	return *(--(path.back()));
      }
      if(sroot.empty()) return NULL;
      path.push_back(sroot.begin());
      return *(path.back());
    }
    LTreeNode *first(){
      if(path.empty()){
	if(sroot.empty()) return NULL;
	path.push_back(sroot.begin());
      } else {
	path.back() = getSiblingList().begin();
      }
      return *(path.back());
    }
    LTreeNode *last(){
      if(path.empty()){
	if(sroot.empty()) return NULL;
	path.push_back(--(sroot.end()));
      } else {
	if(getSiblingList().empty()) mgrThrow(ERR_INT_STATE);
	path.back() = --(getSiblingList().end());
      }
      return *(path.back());
    }
	
};

#endif // _UTIL_LTREE_H_
