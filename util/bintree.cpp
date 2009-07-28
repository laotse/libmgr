/*
 *
 * Binary trees
 *
 * (c) 2005 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: bintree.cpp,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  mgrList - generic doubly linked list
 *  mgrNode - nodes for this list
 *
 * This defines the values:
 *
 */

#define DEBUG_ALL  (0)
#define DEBUG_SWAP (1 << 0)
#define DEBUG_SORT (1 << 1)
#define DEBUG_CTOR (1 << 2)
//#define DEBUG (DEBUG_CTOR)

#include "mgrDebug.h"

#include "bintree.h"
#include "bintree.tag"

/*
 *
 * The leaf class
 *
 *************************************************************
 *
 */

static inline void swap_pointers(__binLeaf **k, __binLeaf **l){
  __binLeaf *t=*k;

  *k = *l;
  *l = t;
}

m_error_t __binLeaf::swap(__binLeaf *l){
  if(!l) return ERR_PARAM_NULL;

  swap_pointers(&(this->parent), &(l->parent));
  swap_pointers(&(this->child[0]), &(l->child[0]));
  swap_pointers(&(this->child[1]), &(l->child[1]));
  
  return ERR_NO_ERROR;
}

m_error_t __binLeaf::add(__binLeaf *l,unsigned const char c){
  if(!l) return ERR_PARAM_NULL;
  if(c > 1) return ERR_PARAM_RANG;
  if(child[c]) return ERR_PARAM_OPT;

  child[c] = l;
  l->parent = this;

  return ERR_NO_ERROR;
}

m_error_t __binLeaf::insert(__binLeaf *l,unsigned const char c){
  if(!l) return ERR_PARAM_NULL;
  if(c > 1) return ERR_PARAM_RANG;
  if(child[c] && l->child[c]) return ERR_PARAM_OPT;

  if(child[c]){
    l->child[c] = child[c];
    child[c]->parent = l;
  }
  child[c] = l;
  l->parent = this;

  return ERR_NO_ERROR;
}

unsigned char __binLeaf::isLeaf(void){
  unsigned char c = 2;
  
  if(child[0]) c--;
  if(child[1]) c--;

  return c;
}

bool __binLeaf::isLinked(void){
  if(!parent) return false;
  if((parent->child[0] == this) ||
     (parent->child[1] == this)) return true;
  return false;
}

bool __binLeaf::isRoot(void){
  if(!isLinked()) return false;
  if(parent->parent) return false;
  return true;
}

/*
 *
 * The Binary Tree class
 *
 ***********************************************************
 *
 */

// all leafs are equal
static int cmp_leafs(__binLeaf *a, __binLeaf *b){
  return 0;
}

__binTree::__binTree(void){
  // __binLeaf root is initialised by its own constructor
  fcmp = cmp_leafs;
}

// a generic iterator, for non-recursive walk of the tree
__binLeaf *__binTree::next(__binLeaf *l, bool back){
  if(!l) return l;
  if(!back){
    // walking down the tree
    if(l->child[0]) return l->child[0];
    if(l->child[1]) return l->child[1];
    // this is the end of this branch
    return NULL;
  }
  // we come from somewhere down the tree and must find our way up
  if(!l->parent) return NULL;
  if(l == l->parent->child[0]){
    // we come up from the left branch, thus go right or up
    if(l->parent->child[1]) return l->parent->child[1];
  }
  // we come up from the right branch
  // pass back through the tree until we arrive at a dual linked leaf,
  // which we find from left
  while(l->parent){
    if(l->parent->child[1]) return l->parent->child[1];
    while(l->parent && (l != l->parent->child[0])) l=l->parent;
    if(l->parent && !l->parent->child[1]) l=l->parent; // no right node, skip
    else break;
  }
  if(!l->parent) return NULL; // no more right nodes - done!
  // we found a right node, go there
  return l->parent->child[1];    
}

m_error_t __binTree::isValid(void){
  __binLeaf *l,*c;

  if(isEmpty()) return ERR_NO_ERROR;
  l = root.child[0];
  if(l->parent != &root) return ERR_INT_DATA;
  do{
    c = next(l,false);
    if(!c) c = next(l,true);
    l = c;
    if(l){
      if(l->child[0] && (l->child[0] == l->child[1])) return ERR_PARAM_UNIQ;
      if(!l->isLinked()) return ERR_INT_DATA;
    }
  } while(l);
    
  return ERR_NO_ERROR;  
}

bool __binTree::isParent(__binLeaf *father, __binLeaf *heir){
  if(!father || !heir) return false;

  while(heir->parent && heir->parent != father) heir=heir->parent;
  if(heir->parent) return true;
  return false;
}

m_error_t __binTree::addLeaf(__binLeaf *leaf,__binLeaf *stem, bool t){

  if(!leaf) return ERR_PARAM_NULL;
  if(!stem){
    if(!isEmpty()) return ERR_PARAM_OPT;
    stem = &root;
    t = false;
  } else {
    if(stem->child[(t)?1:0]) return ERR_PARAM_OPT;
  }
  
  if(leaf->isLinked()) return ERR_PARAM_UNIQ;

  stem->child[(t)?1:0] = leaf;
  leaf->parent = stem;

  return ERR_NO_ERROR;
}

m_error_t __binTree::insertLeaf(__binLeaf *leaf,__binLeaf *stem, bool t){
  unsigned char i;

  if(!leaf || !stem) return ERR_PARAM_NULL;
  if(leaf->isLinked()) return ERR_PARAM_UNIQ;  
  if(!stem->parent) return ERR_PARAM_OPT;
  
  for(i=0;i<2 && stem->parent->child[i] != stem;i++);
  if(i > 1) return ERR_INT_DATA;
  
  stem->parent->child[i] = leaf;
  leaf->parent = stem->parent;
  stem->parent = leaf;  
  leaf->child[(t)?1:0] = stem;
  
  return ERR_NO_ERROR;
}

const char * __binTree::VersionTag(void){
  return _VERSION_;
}

/*
 *
 * The Test-Suite
 *
 ************************************************************************
 *
 */

#ifdef TEST
# include <stdio.h>
int main(int argc, const char *argv[]){
  int errors,tests,res;

  tests = errors = 0;
  
  printf("Test %d: Leaf creation\n",++tests);
  __binLeaf leaves[10];
  for(res=0;res<10;res++){
    if(leaves[res].left() || leaves[res].right() || leaves[res].up()){
      errors++;
      printf("*** Error: Node %d not properly initialised!\n",res+1);
      break;
    }
  }

  printf("Test %d: Tree creation\n",++tests);
  __binTree btree;
  do{
    if(!btree.isEmpty()){
      errors++;
      printf("*** Error: New tree not empty\n");
      break;
    }
    if(ERR_NO_ERROR != (res=btree.isValid())){
      errors++;
      printf("*** Error: New tree is corrupted: 0x%.4x\n",res);
      break;
    }
  } while(0);

  printf("Test %d: Add first Leaf\n",++tests);
  do {
    res = btree.addHead(&(leaves[0]));
    //res = btree.addLeaf(&(leaves[0]),btree.getRoot(),false);
    if (ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: Adding leaf failed: 0x%.4x\n",res);
      break;      
    }
    if(btree.isEmpty()){
      errors++;
      printf("*** Error: Tree is empty after adding leaf\n");
      break;
    }
    if(ERR_NO_ERROR != (res=btree.isValid())){
      errors++;
      printf("*** Error: Tree is corrupted after adding leaf: 0x%.4x\n",res);
      break;
    }    
  } while(0);


  printf("\n%d tests completed with %d errors.\n",tests,errors);
  printf("Used version: %s\n",btree.VersionTag());

  return 0;
}
#endif //TEST
