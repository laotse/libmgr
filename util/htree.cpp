/*
 *
 * Hierarchical Trees
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: htree.cpp,v 1.6 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  HTreeNode - simple skeleton node for HTrees
 *  HTree     - actually an iterator referencing a root HTreeNode
 *
 * This defines the values:
 *
 */

#include "htree.h"
#include "htree.tag"

#define DEBUG_DUMP 1
#define DEBUG_MARK 2
#define DEBUG (DEBUG_DUMP | DEBUG_MARK)
#ifdef DEBUG
# include <stdio.h>
#endif
#include <mgrDebug.h>

using namespace mgr;

void HTree::initTree(HTreeNode *root){
    path.clear();
    path.push_back(root);
    sroot = root;    
}

// These ones are protected for internal use only
void HTree::insertChild(class HTreeNode *parent, class HTreeNode *node) const {
  node->next = parent->child;
  parent->child = node;
}

void HTree::insertNext(class HTreeNode *precessor, class HTreeNode *node) const {
  node->next = precessor->next;
  precessor->next = node;
}

/*! If the current() path is undefined insertChild()
    is treated as insertNext().

    It is possible to use insertChild() for inserting
    entire sub-trees.
    If moveCurrent is selected the current path will
    be set to the last node in the top-level sequence 
    starting at c.
*/
HTreeNode *HTree::insertChild(HTreeNode *c, bool moveCurrent){
  HTreeNode *p = current(), *t;

  if(!p) return insertNext( c, moveCurrent );

  if(p->child || moveCurrent){
    for(t=c;t->next;t=t->next);
    //if(p->child) 
    t->next = p->child;
  } else t=NULL; // keep g++ from crying
  p->child = c;
  if(moveCurrent){
    child();
    path.back() = t;
    p = t;
  }
  return p;
}

/*! If the current() path is undefined c is inserted
    before the root node, if exists. 

    It is possible to use insertNext() for inserting
    entire sub-trees.
    If moveCurrent is selected the current path will
    be set to the last node in the top-level sequence 
    starting at c.

    If moveCurrent is not selected and NULL is returned,
    this means that the sub-tree c has been inserted
    before the root node, i.e. c is the new root.
 */
HTreeNode *HTree::insertNext(HTreeNode *c, bool moveCurrent){
  HTreeNode *p = current(), *t;

  if(!p){
    if(sroot){
      for(t=c;t->next;t=t->next);
      t->next = sroot;
      sroot = c;
      if(moveCurrent){
	root();
	path.back() = t;
      } else t = NULL;
      return t;
    }
    initTree(c);
    if(moveCurrent){
      for(t=root();t->next;t=t->next);
      path.back() = t;
    } else t = NULL;
    return t;
  }

  if(p->next || moveCurrent){
    for(t=c;t->next;t=t->next);
    //if(p->next) 
    t->next = p->next;
  } else t = NULL; // keep g++ from crying
  p->next = c;
  if(moveCurrent){
    path.back() = t;
    p = t;
  }
  return p;
}

/*! If the current() path is undefined appendChild()
    is treated as appendNext().

    It is possible to use appendChild() for appending
    entire sub-trees.
    If moveCurrent is selected the current path will
    be set to c.
*/
HTreeNode *HTree::appendChild(HTreeNode *c, bool moveCurrent){
  HTreeNode *p = current(), *t;

  if(!p) return appendNext( c, moveCurrent );

  if(p->child){
    for(t=p->child;t->next;t=t->next);
    t->next = c;
  } else insertChild(c);
  if(moveCurrent){
      child();
      path.back() = c;
  }
  
  return current();
}

/*! If the current() path is undefined c is inserted
    at the end of the root sequence, if exists. If the 
    tree is empty it is initialised with the new contents.

    It is possible to use appendNext() for appending
    entire sub-trees.
    If moveCurrent is selected the current path will
    be set to c.

    If moveCurrent is not selected and NULL is returned,
    this means that the sub-tree c has been inserted
    after the root node. In contrast to insertNext()
    this does not imply that c is the new root node.
 */
HTreeNode *HTree::appendNext(HTreeNode *c, bool moveCurrent){
  HTreeNode *p = current();   
    
  if(!p){
    if(sroot){
      for(p=sroot;p->next;p=p->next);
      p->next = c;
      if(moveCurrent){
	root();
	path.back()=c;
      }
      return current();
    }
    initTree(c);
    if(moveCurrent) return root();
    return NULL;
  }
  
  for(;p->next;p=p->next);
  p->next = c;
  if(moveCurrent) path.back() = c;
  
  return current();   
}

HTreeNode *HTree::firstSibling(void){
    if(path.size() <= 1){
	if(sroot){
	    path.clear();
	    path.push_back(sroot);
	}
	return sroot;
    }
    class HTreeNode *p = path[path.size()-2]->child;
    path.back() = p;
    
    return current();
}

HTreeNode *HTree::iterate(int *depth){
  HTreeNode *p = current();

  if(p->child){
    child();
    if(depth) *depth = path.size();
    return current();
  }

  if(p->next){
    next();
    if(depth) *depth = path.size();
    return current();
  }

  while((p=parent()) && (!p->next));
 
  if(!p){
    root();    
    // p is NULL and stays NULL for return
    if(depth) *depth = -1;
  } else {
    p = next();
    if(depth) *depth = path.size();
  }

  return p;
}

/*! This function removes the current node from the HTree linkage.
    If the node itself has children these stay attached to this node.
    The pointer to the removed node is returned.

    If NULL is returned the tree is either empty or there is a
    linkage problem in the HTree.
*/
HTreeNode *HTree::slice(void){
    HTreeNode *r = current();
    
    if(r == sroot || path.empty()){
	path.clear();
	sroot = NULL;
	if(r) r->next = NULL;
	return r;
    }
    class HTreeNode *p = firstSibling();
    if(p == r){
	p = parent();
	p->child = r->next;
	r->next = NULL;
	return r;
    }
    for(;p->next != r && p->next;p=p->next);
    // should never happen
    if(!p->next) return NULL;
 
    p->next = r->next;
    r->next = NULL;
    path.back() = p;
    
    return r;
}

// freeNode() is virtual and will serve the real nodes in the tree
void HTree::remove(HTreeNode *c, bool rfree) const {
  if(!c) return;      
  do {
    if(c->child) remove(c->child,true);
    c->child = NULL;
    HTreeNode *n = c->next;
    c->next = NULL;
    if(rfree) {
      freeNode(c);
    }
    c = n;
  } while( c );
}

// copyNode() is virtual and will serve the real nodes in the tree
HTreeNode *HTree::copy(HTreeNode *n) const {
  xpdbg(MARK,"### HTree::copy() called\n");
  if(!n) n = current();
  if(!n) return n;

  m_error_t err = ERR_NO_ERROR;
  xpdbg(MARK,"### copy root_node\n");
  HTreeNode *r = copyNode(n);
  HTreeNode *c = r;
  while(c && n){
    if(n->child){
      c->child = copy(n->child);
      if(!c->child){
	err = ERR_MEM_AVAIL;
	break;
      }
    }
    if(n->next){
      xpdbg(MARK,"### copy next\n");
      c->next = copyNode(n->next);
      if(!c->next){
	err = ERR_MEM_AVAIL;
	break;
      }
    }
    c = c->next;
    n = n->next;
  }
  if(err != ERR_NO_ERROR){
    remove(r,true);
    return NULL;
  }

  return r;
}

// deep copy of entire HTree

m_error_t HTree::clone(const HTree& t){
  HTreeNode *r = copy(t.sroot);
  if(!r) return ERR_MEM_AVAIL;

  sroot = r;
  path.clear();
  HTreeNode *q = t.sroot;
  
  xpdbg(DUMP,"htree::clone() path loop for %d\n",t.path.size());
  m_error_t err = ERR_NO_ERROR;
  for(size_t i=0;i<t.path.size();i++){
    if(i){
      q = q->child;
      r = r->child;
    }
    xpdbg(DUMP,"htree::clone() path[%d] of %d\n",i,t.path.size());
    while(q && q != t.path.at(i)){
      q = q->next;
      if(r) r = r->next;
      else err = ERR_INT_DATA;
    }
    if(!q) err = ERR_INT_DATA;
    if(err != ERR_NO_ERROR) break;
    path.push_back(r);
  }
  if(err != ERR_NO_ERROR){
    remove(sroot,true);
    path.clear();
    sroot = NULL;
  }

  return err;
}

// Bookmark support
ssize_t HTree::pathValidDepth( const Bookmark& p ) const {
  if(! p.size() ) return 0;    
  const HTreeNode *n = sroot;
  while(n && ( n != p[0] )) n = n->next;
  if(!n) return 0;
  size_t s = 1;
  for(; s < p.size(); ++s){
    if( !isChildOf(p[s], n )){
      return -s;
    }
    n = p[s];
  }
  return s;
}


/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <stdio.h>

class tNode : public HTreeNode {
public:
  const char *name;

  tNode(){
    name = NULL;
  }

  tNode(const char *n){
    name = n;
  }

  const char *dump(){
    if(name) return name;
    return "UNNAMED";
  }
    
};


int main(int argc, char *argv[]){
  int depth;
  
  pdbg("Started!\n");
  tNode *r = new tNode("Root");
  pdbg("Created Root!\n");
  HTree *t = new HTree(r);
  pdbg("Created Tree at depth %d!\n",t->depth());
  tNode *n = new tNode("Next");
  pdbg("Created Node 1!\n");
  t->insertNext(n);
  t->next();
  pdbg("Added Node 1!\n");
  n = new tNode("Child of Next");
  t->insertChild(n);
  t->child();
  pdbg("Added Node 1's child at depth %d!\n",t->depth());
  t->parent();
  pdbg("Parent, now at depth: %d\n",t->depth());
  n = new tNode("Next 2");
  pdbg("Created Node 2!\n");
  t->insertNext(n);
  pdbg("Added Node 2!\n");


  printf("\nDump tree:\n");
  n = static_cast<tNode *>(t->root());
  depth = t->depth();
  while(n){
    printf("%d: %s\n",depth,n->dump());
    n = static_cast<tNode *>(t->iterate(&depth));
  }

  printf("\n Clearing tree\n");
  t->clear();
  n = new tNode("After Clear Node");
  pdbg("Created new node!\n");
  t->insertChild(n);
  pdbg("Inserted as Child\n");
  
  return 0;
}

#endif // TEST
