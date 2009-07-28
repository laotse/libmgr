/*
 *
 * Doubly linked lists
 *
 * (c) 2005 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: lists.cpp,v 1.5 2008-05-15 20:58:25 mgr Exp $
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

#include "lists.h"
#include "lists.tag"


/*
 * The node class
 **********************************************************
 *
 */

// Constructor
__mgrNode::__mgrNode(void){
  next = (__mgrNode *)NULL;
  last = (__mgrNode *)NULL;
  xpdbg(CTOR,"...initialise __mgrNode orphan 0x%lx\n",this);
}

// Initialiser
__mgrNode::__mgrNode(const __mgrNode&){
  // we do not mess with the list linkage, use swap or something similar  
  xpdbg(CTOR,"...empty __mgrNode copy constructor called\n");
}  

// Assigment
__mgrNode& __mgrNode::operator=(const __mgrNode&){
  // we do not mess with the list linkage, use swap or something similar
  xpdbg(CTOR,"...empty __mgrNode assign operator called\n");
  return *this;
}

bool __mgrNode::isLinked(void){
  if(last && next) return true;
  return false;
}

bool __mgrNode::isFirst(void){
  if(!last){
    // This is not a linked node!
    return true;
  }
  if(last->last) return false;
  return true;
}

bool __mgrNode::isLast(void){
  if(!next){
    // This is not a linked node!
    return true;
  }
  if(next->next) return false;
  return true;
}

__mgrNode *__mgrNode::succ(void){
  if(!this) return (__mgrNode *)NULL;
  return next;
}

__mgrNode *__mgrNode::pred(void){
  if(!this) return (__mgrNode *)NULL;
  return last;
}

m_error_t __mgrNode::prepend(__mgrNode *n){
  if(!isLinked() || !n) return ERR_PARAM_NULL;
  n->next = this;
  n->last = last;
  last = n;
  n->last->next = n;

  return ERR_NO_ERROR;
}

m_error_t __mgrNode::postpend(__mgrNode *n){
  if(!isLinked() || !n) return ERR_PARAM_NULL;
  n->last = this;
  n->next = next;
  next = n;
  n->next->last = n;

  return ERR_NO_ERROR;
}

m_error_t __mgrNode::unlink(void){
  if(!isLinked()) return ERR_PARAM_NULL;

  last->next = next;
  next->last = last;
  next = (__mgrNode *) NULL;
  last = (__mgrNode *) NULL;

  return ERR_NO_ERROR;
}

m_error_t __mgrNode::linkage(void){
  if(!isLinked()) return ERR_CANCEL;

  if(this->next->last != this) return ERR_INT_DATA;
  if(this->last->next != this) return ERR_INT_DATA;

  return ERR_NO_ERROR;
}

m_error_t __mgrNode::swap(__mgrNode *n){
  __mgrNode *p,*q;
  int b;

  if(!n) return ERR_PARAM_NULL;
  if(isLinked() && n->isLinked()){
    xpdbg(SWAP,"Swap: %x(%x,%x) <> %x(%x,%x)\n",
	  this,this->next,this->last,
	  n,n->next,n->last);
    if((b = buddy(n))){
      // need specific implemenatation to swap neighbours
      if(b<0){
	p = n;
	q = this;
      } else {
	p = this;
	q = n;
      }
      // p preceeds q (p->next == q) and these shall be swapped
      p->next = q->next;
      q->last = p->last;
      p->next->last = p;
      q->last->next = q;
      p->last = q;
      q->next = p;
    } else {
      this->last->next=n;
      this->next->last=n;
      n->last->next=this;
      n->next->last=this;
      p = this->next;
      q = this->last;
      this->next = n->next;
      this->last = n->last;
      n->next = p;
      n->last = q;
    }
    xpdbg(SWAP,"Result: %x(%x,%x) <> %x(%x,%x)\n",
	  this,this->next,this->last,
	  n,n->next,n->last);
    return ERR_NO_ERROR;
  }

  if(!isLinked() && !(n->isLinked()))
    return ERR_PARAM_OPT;

  if(isLinked()){
    // swap out this node with unlinked external
    n->next = this->next;
    n->last = this->last;
    this->next->last = n;
    this->last->next = n;
    this->next = (__mgrNode *)NULL;
    this->last = (__mgrNode *)NULL;
    return ERR_NO_ERROR;
  } else return n->swap(this);
}

int __mgrNode::buddy(__mgrNode *n){
  if(next == n) return 1;
  if(last == n) return -1;
  return 0;
}

/*
 *
 * The list class
 *******************************************************
 *
 */

static int cmp_dummy(__mgrNode *n, __mgrNode *m){
  return 0;
}

__mgrList::__mgrList(void){
  head.next = &tail;
  head.last = (__mgrNode *)NULL;
  tail.next = (__mgrNode *)NULL;
  tail.last = &head;
  set_cmp(cmp_dummy);
  xpdbg(CTOR,"...created empty __mgrList\n");
}

// copy constructor
__mgrList::__mgrList(const __mgrList& l){
  __mgrNode *n = head.next;
  __mgrNode *p;

  xpdbg(CTOR,"...copying __mgrList\n");
  while(n->next){
    p = new __mgrNode;
    *p = *n;
    ((__mgrList *)&l)->addTail(p);
    n = n->next;
  }
  xpdbg(CTOR,"...copying __mgrList done\n");
}

m_error_t __mgrList::set_cmp(int (*new_cmp)(__mgrNode *,__mgrNode *)){
  fcmp = new_cmp;
  return ERR_NO_ERROR;
}

int __mgrList::cmp(__mgrNode *n, __mgrNode *m){
  return fcmp(n, m);
}

bool __mgrList::isEmpty(void){
  if(head.next == &tail) return true;
  return false;
}

bool __mgrList::isSequence(__mgrNode *n, __mgrNode *m){
  __mgrNode *t = n;

  if(!n || !m) return false;
  while(t->next && (t != m)) t=t->next;
  
  return (t == m);
}

__mgrNode *__mgrList::getHead(void){
  if(isEmpty()) return (__mgrNode *) NULL;
  return head.next;
}

__mgrNode *__mgrList::getTail(void){
  if(isEmpty()) return (__mgrNode *) NULL;
  return tail.last;
}

m_error_t __mgrList::addHead(__mgrNode *n){
  m_error_t err;
  __mgrNode *h = getHead();

  if(h){
    err = h->prepend(n);
    return err;
  }
  // List is empty!
  n->next = &tail;
  tail.last = (__mgrNode *)n;
  n->last = &head;
  head.next = (__mgrNode *)n;

  return ERR_NO_ERROR;
}

m_error_t __mgrList::addTail(__mgrNode *n){
  m_error_t err;
  __mgrNode *h = getTail();

  if(h){
    err = h->postpend(n);
    return err;
  }
  // List is empty!
  n->next = &tail;
  tail.last = (__mgrNode *)n;
  n->last = &head;
  head.next = (__mgrNode *)n;

  return ERR_NO_ERROR;
}

m_error_t __mgrList::isValid(void){
  __mgrNode *n;
  m_error_t res;

  // The stop marks shall never change!
  if(head.last || tail.next) return ERR_INT_STATE;
  // Empty list, with good double links is okay!
  if((head.next == &tail) && (tail.last == &head)) 
    return ERR_NO_ERROR;
  // There should be a node seen from both ends
  if(head.next == &tail) return ERR_INT_DATA;
  if(tail.last == &head) return ERR_INT_DATA;
  if(!head.next || !tail.last) return ERR_INT_STATE;
  // Now walk the chain
  
  n = getHead();
  while(n->next){
    if(ERR_NO_ERROR != (res = n->linkage()))
      return res;
    n = n->next;
  };

  return ERR_NO_ERROR;
}

m_error_t __mgrList::clear(bool free){
  __mgrNode *n,*m;
  m_error_t res;

  if(isEmpty()) return ERR_NO_ERROR;
  n = getHead();
  while(n->next){
    m = n->next;
    if(ERR_NO_ERROR != (res=n->unlink())){
      return res;
    }
    if(free) delete n;
    n = m;
  }
  
  return ERR_NO_ERROR;
}

m_error_t __mgrList::clear(void){
  return clear(false);
}

m_error_t __mgrList::purge(void){
  return clear(true);
}

size_t __mgrList::count(void){
  __mgrNode *n;
  size_t c = 0;

  if(isEmpty()) return 0;
  n = getHead();
  while(n->next){
    c++;
    n=n->next;
  }

  return c;
}

__mgrNode * __mgrList::operator[](size_t k){
  __mgrNode *n;
  size_t c = 0;

  if(isEmpty()) return (__mgrNode *)NULL;
  n = getHead();
  while(n->next){
    if(c == k) return n;
    c++;
    n=n->next;
  }

  return (__mgrNode *)NULL;
}

const char * __mgrList::VersionTag(void){
  return _VERSION_;
}

m_error_t __mgrList::sort(__mgrNode *b, __mgrNode *t){
  __mgrNode *ct=t;
  __mgrNode *cb,*tt,*start,*end;
  m_error_t res;  

  xpdbg(SORT,"Sort area: (%lx) %lx - %lx\n",
	(long int)b,(long int)(b->succ()),(long int)t);

  if(b == t) return ERR_NO_ERROR;
  if(!b || !t) return ERR_PARAM_NULL;
  cb = b->succ();
  start = b->pred();
  end = t->succ();

# if DEBUG_CHECK(SORT)
  pdbg("Sorting: ");
  for(tt=b;tt!=t;tt=tt->succ()){
    pdbg("%lx(%d) ",(long int)tt,cmp(b,tt));
  }
  pdbg("%lx(%d)\n",(long int)tt,cmp(b,tt));
# endif

  if(!cb) return ERR_PARAM_RANG;

  if(cb == ct){
    if(cmp(b,ct) > 0) {
      b->swap(ct);
#if DEBUG_CHECK(SORT)
      pdbg("Swapped pair - end of recursion!\n");
    } else {
      pdbg("Kept pair - end of recursion!\n");
#endif
    }
    return ERR_NO_ERROR;
  }

  while(cb != ct){
    while((cb != ct) && (cmp(b,ct) <= 0)) ct=ct->pred(); 
    while((cb != ct) && (cmp(b,cb) >= 0)) cb=cb->succ(); 
    if(cb != ct){
      xpdbg(SORT,"Swapping %lx <> %lx (%d,%d)\n",
	     (long int)cb,(long int) ct, cmp(b,cb), cmp(b,ct));
      // swap position
      cb->swap(ct);
      // swap back handles
      tt = cb;
      cb = ct;
      ct = tt;
    } else {
      if(ct->pred() == b){
      // all elements following b are larger than b
      // nothing to do, b is perfect
	xpdbg(SORT,"Pivot is fine, no swap!\n");
      } else {
	// put pivot to final destination
	xpdbg(SORT,"Swapping pivot %lx <> %lx\n",(long int)b,(long int)ct);
	b->swap(ct);
#if DEBUG_CHECK(SORT)
	pdbg("After swap %lx <> %lx\n",(long int)b,(long int)ct);
	res = isValid();
	if(ERR_NO_ERROR != res){
	  pdbg("list corrupt: 0x%.4x\n",res);
	} else {
	  pdbg("list integer\n");
	}
#endif 
      }
    }
  }
  if(b->pred() != start){
    res = sort(start->succ(),b->pred());
    if(res != ERR_NO_ERROR) return res;
  }
#if DEBUG_CHECK(SORT)
  else {
    pdbg("No left group!\n");
  }
  pdbg("-> go right:");
  tt = getHead();
  while(tt->succ()){
    pdbg(" %lx",tt);
    tt=tt->succ();
  }
  pdbg("\n");
#endif
  if(b->succ() != end){
    res = sort(b->succ(),end->pred());
  } else {
    res = ERR_NO_ERROR;
    xpdbg(SORT,"No right group!\n");
  }

  return res;
}

/*
 *
 * The list walker class
 *******************************************************
 *
 */

__mgrListWalk::__mgrListWalk(__mgrList *l){
  list = l;
  rewind();
}

__mgrNode * __mgrListWalk::rewind(void){
  current = list->getHead()->pred();
  return current->succ();
}

__mgrNode * __mgrListWalk::operator()(void){
  if(current && current->succ()) current = current->succ();
  else return (__mgrNode *)NULL;
  if(!current->succ()) return (__mgrNode *)NULL;
  return current;    
}

/*
 **************************************************************
 *
 * Internal test suite
 *
 * set -DTEST for compile, e.g.:
 *  gcc -o test-lists -DTEST lists.cpp -lstdc++
 *
 **************************************************************
 *
 */

#ifdef TEST

#include <stdio.h>

class intNode : public mgrNode<intNode>{
  int value;

public:
  intNode() : mgrNode<intNode>::mgrNode(){
    value = 0;
    xpdbg(CTOR,"...create mgrNode(), i.e. 0\n");
  }

  //intNode(int v) : mgrNode<intNode>::mgrNode(){
  intNode(int v){
    value = v;
    xpdbg(CTOR,"...create mgrNode(%d) at 0x%lx\n",v,this);
  }

  // initialisation (copy constructor)
  intNode(const intNode& n) : mgrNode<intNode>::mgrNode(){
    value = n.value;
    xpdbg(CTOR,"...copy mgrNode(&d)\n",value);
  }
  
  // assignment
  intNode& operator=(const intNode& n){
    value = n.value;
    xpdbg(CTOR,"...assign mgrNode(%d)\n",value);
    return *this;
  }

  inline int getValue(void){
    return value;
  }

  void dump(void){
    printf(" %d",value);
  }

};

static int int_cmp(__mgrNode *mna, __mgrNode *mnb){
  intNode *a = (intNode *)mna;
  intNode *b = (intNode *)mnb;
  
  if(!a || !b) return -1;
  return -(a->getValue() - b->getValue());
}


int main(int argc, const char *argv[]){
  int errors,tests,res;

  tests = errors = 0;
  
  tests++;
  printf("Test %d: Node creation\n",tests);
  __mgrNode node;
  if(node.isLinked()){
    puts("*** ERROR: Node created is linked!");
    errors++;
  } else {
    puts("+++ Node creation OK");
  }

  tests++;
  printf("Test %d: List creation\n",tests);
  __mgrList list;
  if(!list.isEmpty()){
    puts("*** ERROR: List created is not empty!");
    errors++;
  } else {
    puts("+++ List creation OK");
  }
  
  tests++;
  printf("Test %d: List validation after create\n",tests);
  res = list.isValid();
  if(res != ERR_NO_ERROR){
    printf("*** ERROR: List created is not valid: 0x%.4x!\n",res);
    errors++;
  } else {
    puts("+++ List validation after create OK");
  }
    
  tests++;
  printf("Test %d: Add to head of empty list\n",tests);
  do{
    res = list.addHead(&node);
    if(res != ERR_NO_ERROR){
      printf("*** Error: Add to head failed 0x%.4x!\n",res);
      errors ++;
      break;
    }
    if(!node.isLinked()){
      puts("*** Error: Node added to head is not linked!");
      errors++;
      break;
    }
    puts("+++ Node is linked!");
    res = node.linkage();
    if(res != ERR_NO_ERROR){
      printf("*** Error: Add to head corrupted node linkage 0x%.4x!\n",res);
      errors++;
      break;
    }
    puts("+++ Node is consistent!");
    res = list.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** ERROR: List is not valid: 0x%.4x! after add to head\n",res);
      errors++;
      break;
    }
    puts("+++ Add head consistent!");
  }while(0);

  tests++;
  printf("Test %d: Add to head of list\n",tests);
  __mgrNode node2;
  do{
      res = list.addHead(&node2);
    if(res != ERR_NO_ERROR){
      printf("*** Error: Add to head failed 0x%.4x!\n",res);
      errors ++;
      break;
    }
    if(!node2.isLinked()){
      puts("*** Error: Node added to head is not linked!");
      errors++;
      break;
    }
    puts("+++ Node is linked!");
    res = node2.linkage();
    if(res != ERR_NO_ERROR){
      printf("*** Error: Add to head corrupted node linkage 0x%.4x!\n",res);
      errors++;
      break;
    }
    puts("+++ Node is consistent!");
    res = list.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** ERROR: List is not valid: 0x%.4x! after add to head\n",res);
      errors++;
      break;
    }
    puts("+++ Add head consistent!");
  }while(0);

  tests++;
  printf("Test %d: Add to tail of list\n",tests);
  __mgrNode node3;
  do{
    res = list.addTail(&node3);
    if(res != ERR_NO_ERROR){
      printf("*** Error: Add to tail failed 0x%.4x!\n",res);
      errors ++;
      break;
    }
    if(!node3.isLinked()){
      puts("*** Error: Node added to tail is not linked!");
      errors++;
      break;
    }
    puts("+++ Node is linked!");
    res = node3.linkage();
    if(res != ERR_NO_ERROR){
      printf("*** Error: Add to tail corrupted node linkage 0x%.4x!\n",res);
      errors++;
      break;
    }
    puts("+++ Node is consistent!");
    res = list.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** ERROR: List is not valid: 0x%.4x! after add to tail\n",res);
      errors++;
      break;
    }
    puts("+++ Add tail consistent!");
  }while(0);

  tests++;
  printf("Test %d: Get head\n",tests);
  __mgrNode *n = list.getHead();
  if(!n){
    puts("*** Error: Get head failed!");
  } else {
    if(n != &node2){
      puts("*** Error: got wrong node!");
      errors++;
    } else {
      puts("+++ Get head OK!");
    }
  }

  tests++;
  printf("Test %d: Remove head node\n",tests);
  n->unlink();
  do{
    if(n->isLinked()){
      puts("*** Error: deleted node is linked!");
      errors++;
      break;
    }
    puts("+++ Deleted node is not linked!");
    if(list.isEmpty()){
      puts("*** Error: List is empty after head remove!");
      errors++;
      break;
    }
    res = list.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** ERROR: List is not valid: 0x%.4x! after head unlink\n",res);
      errors++;
      break;
    }    
    puts("+++ Head removed OK!");
  }while(0);

  tests++;
  printf("Test %d: Count\n",tests);
  size_t c;
  c=list.count();
  if(c!=2){
    printf("*** Error: List counted %d - should be 2\n",c);
    errors++;
  } else {
    puts("+++ List count OK!");
  }

  tests++;
  printf("Test %d: Get Tail\n",tests);
  n = list.getTail();
  if(!n){
    puts("*** Error: Get tail failed!");
  } else {
    if(n != &node3){
      puts("*** Error: got wrong node!");
      errors++;
    } else {
      puts("+++ Get tail OK!");
    }
  }

  tests++;
  printf("Test %d: Remove Tail successor\n",tests);
  res = n->succ()->unlink();
  if(res != ERR_PARAM_NULL){
    printf("*** Error: unlink did not fail as expected, but returned: 0x%.4x!\n",res);
    errors++;
  } else {
    puts("+++ unlink failed OK!");
  }

  tests++;
  printf("Test %d: Remove Tail precessor\n",tests);
  res = n->pred()->unlink();
  if(res != ERR_NO_ERROR){
    printf("*** Error: unlink failed: 0x%.4x!\n",res);
    errors++;
  } else {
    res=list.isValid();
    if(res == ERR_NO_ERROR){
      puts("+++ unlink OK!");
    } else {
      errors++;
      printf("*** Error: List inconsistent after unlink: 0x%.4x!\n",res);
    }
  }

  tests++;
  printf("Test %d: Remove final node\n",tests);
  res = n->unlink();
  do {
    if(res != ERR_NO_ERROR){
      printf("*** Error: unlink failed: 0x%.4x!\n",res);
      errors++;
      break;
    }
    res = list.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** Error: unlink corrupted list: 0x%.4x!\n",res);
      errors++;
      break;
    }
    if(!list.isEmpty()){
      printf("*** Error: List not empty after removal of final node!\n");
      errors++;
      break;
    }
    puts("+++ unlink OK!");
  } while(0);

  /*
  tests++;
  printf("Test %d: Detect list corruption\n",tests);
  puts("... Rebuilding list");
  do {
    res = list.addTail(&node);
    if(res != ERR_NO_ERROR){
      printf("*** Error: adding node 1 failed: 0x%.4x!\n",res);
      errors++;
      break;
    }
    res = list.addTail(&node2);
    if(res != ERR_NO_ERROR){
      printf("*** Error: adding node 2 failed: 0x%.4x!\n",res);
      errors++;
      break;
    }
    res = list.addTail(&node3);
    if(res != ERR_NO_ERROR){
      printf("*** Error: adding node 3 failed: 0x%.4x!\n",res);
      errors++;
      break;
    }
    res = list.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** Error: inconsistent list after adds: 0x%.4x!\n",res);
      errors++;
      break;
    }
    puts("... corrupting list!");
    n = node.succ();
    // must make next public to do this!
    node.next = &node3;
    res = list.isValid();
    if(res == ERR_NO_ERROR){
      printf("*** Error: consistent list after corruption: 0x%.4x!\n",res);
      errors++;
      break;
    } 
    printf("+++ corruption detected OK: 0x%.4x!\n",res);
    node.succ() = n;
    res = list.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** Error: inconsistent list after fixing: 0x%.4x!\n",res);
      errors++;
      break;
    }
  }while(0);
  */

  tests++;
  printf("Test %d: mgrList::clear() - unlink all nodes\n",tests);
  res = list.clear();
  do{
    if(res != ERR_NO_ERROR){
      printf("*** Error: clear failed: 0x%.4x\n",res);
      errors++;
      break;
    }
    res = list.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** Error: clear corrupted list: 0x%.4x\n",res);
      errors++;
      break;
    }
    if(!list.isEmpty()){
      puts("*** Error: list not empty after clear!");
      errors++;
      break;
    }
    puts("+++ test clear() OK!");
  }while(0);

  tests++;
  printf("Test %d: Dynamic nodes and purge()\n",tests);
  puts("... Rebuilding list with dynamic nodes");
  do {
    n = new __mgrNode;
    res = list.addTail(n);
    if(res != ERR_NO_ERROR){
      printf("*** Error: adding node 1 failed: 0x%.4x!\n",res);
      errors++;
      break;
    }
    n = new __mgrNode;
    res = list.addTail(n);
    if(res != ERR_NO_ERROR){
      printf("*** Error: adding node 2 failed: 0x%.4x!\n",res);
      errors++;
      break;
    }
    n = new __mgrNode;
    res = list.addTail(n);
    if(res != ERR_NO_ERROR){
      printf("*** Error: adding node 3 failed: 0x%.4x!\n",res);
      errors++;
      break;
    }
    res = list.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** Error: inconsistent list after adds: 0x%.4x!\n",res);
      errors++;
      break;
    }
    res = list.purge();
    if(res != ERR_NO_ERROR){
      printf("*** Error: purge failed: 0x%.4x\n",res);
      errors++;
      break;
    }
    res = list.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** Error: purge corrupted list: 0x%.4x\n",res);
      errors++;
      break;
    }
    if(!list.isEmpty()){
      puts("*** Error: list not empty after purge!");
      errors++;
      break;
    }
    puts("+++ test purge() OK!");    
  }while(0);

  tests++;
  printf("Test %d: Container class allocation\n",tests);
  mgrList<intNode> ilist;
  do{
    res = ilist.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** Error: container list inconsistent: 0x%.4x\n",res);
      errors++;
      break;
    }
    if(!ilist.isEmpty()){
      puts("*** Error: container list not empty");
      errors++;
      break;
    }
    puts("+++ Container list created OK!");
  }while(0);

  tests++;
  printf("Test %d: Container class compliance\n",tests);
  puts("... Building list with dynamic, inherited nodes");
  intNode *in;
  for(int i=0;i<10;i++){
    in = new intNode(i);
    res = ilist.addHead(in);
    if(res != ERR_NO_ERROR){
      printf("*** Error: adding node %d: 0x%.4x\n",i+1,res);
      errors++;
      break;
    }
  }
  do {
    res=ilist.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** Error: list inconsistent: 0x%.4x\n",res);
      errors++;
      break;
    }
    c = ilist.count();
    if(c != 10){
      printf("*** Error: list contains %d items, but should have 10\n",c);
      errors++;
      break;
    }
    printf("+++ Build inherited nodes' list OK!");
    in = ilist.getHead();
    while(in->succ()){
      in->dump();
      in = in->succ();
    }
    putchar('\n');
  }while(0);

  tests++;
  printf("Test %d: Iterator Test\n",tests);
  mgrListWalk<intNode> iwalk(&ilist);
  do {
    printf("+++ Dumping with iterator:");
    while((in = iwalk())) in->dump();
    putchar('\n');
  } while(0);

  tests++;
  printf("Test %d: Swap test internal\n",tests);
  do{
    res = ilist.getHead()->swap(ilist.getTail());
    if(res != ERR_NO_ERROR){
      printf("*** Error: swap failed: 0x%.4x\n",res);
      errors++;
      break;
    }
    res = ilist.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** Error: list inconsisten after swap: 0x%.4x\n",res);
      errors++;
      break;
    }
    printf("+++ OK: dumping result:");
    iwalk.rewind();
    while((in = iwalk())) in->dump();
    putchar('\n');    
  } while(0);

  tests++;
  printf("Test %d: Swap test external\n",tests);
  do{
    in = new intNode(20);
    intNode *sin;
    sin = ilist.getHead()->succ();
    res = sin->swap(in);
    if(res != ERR_NO_ERROR){
      printf("*** Error: swap failed: 0x%.4x\n",res);
      errors++;
      break;
    }
    res = ilist.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** Error: list inconsisten after swap: 0x%.4x\n",res);
      errors++;
      break;
    }
    if(sin->isLinked()){
      puts("*** Error: old node still linked!");
      errors++;
      break;
    }
    delete sin;
    printf("+++ OK: dumping result:");
    iwalk.rewind();
    while((in = iwalk())) in->dump();
    putchar('\n');    
  } while(0);

  tests++;
  printf("Test %d: Swap test external reverse\n",tests);
  do{
    in = new intNode(21);
    intNode *sin;
    sin = ilist.getTail()->pred();
    res = in->swap(sin);
    if(res != ERR_NO_ERROR){
      printf("*** Error: swap failed: 0x%.4x\n",res);
      errors++;
      break;
    }
    res = ilist.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** Error: list inconsisten after swap: 0x%.4x\n",res);
      errors++;
      break;
    }
    if(sin->isLinked()){
      puts("*** Error: old node still linked!");
      errors++;
      break;
    }
    delete sin;
    printf("+++ OK: dumping result:");
    iwalk.rewind();
    while((in = iwalk())) in->dump();
    putchar('\n');    
  } while(0);

  tests++;
  printf("Test %d: Simple compare test\n",tests);
  // Set ordering criterion
  ilist.set_cmp(int_cmp);
  iwalk.rewind();
  intNode *sin = iwalk();
  while((in = iwalk())){
    printf("??? cmp:");
    sin->dump();
    in->dump();
    printf(" -> %d\n",ilist.cmp(sin,in));
    sin = in;
  }

  tests++;
  printf("Test %d: Array operator test\n",tests);
  printf("??? Dump: ");
  for(int i=0;(size_t)i<ilist.count();i++){
    if(ilist[i]){
      ilist[i]->dump();
    } else {
      errors++;
      printf(" --> premature end of array at %d",i);
      break;
    }
  }
  putchar('\n');

  tests++;
  printf("Test %d: Sort test\n",tests);
  iwalk.rewind();
  printf("--- start:");
  while((in = iwalk())){
    in->dump();
  }
  putchar('\n');
  do{
    res = ilist.sort();
    if(res != ERR_NO_ERROR){
      printf("*** Error: sort failed: 0x%.4x\n",res);
      errors++;
      break;
    }
    puts("+++ Sort completed!");
    iwalk.rewind();
    printf("???  sort:");
    while((in = iwalk())){
      in->dump();
    }
    putchar('\n');
  }while(0);

  tests++;
  printf("Test %d: isSequence()\n",tests);
  in = ilist.getHead();
  for(res=0;res<3;res++) in=in->succ();
  sin = in;
  for(res=0;res<3;res++) in=in->succ();
  do{
    if(!ilist.isSequence(sin,in)){
      puts("*** Error: Correct sequence is not acknowledged as sequence!");
      errors++;
      break;
    }
    if(ilist.isSequence(in,sin)){
      puts("*** Error: Wrong sequence is acknowledged as correct!");
      errors++;
      break;
    }
    puts("+++ Sequence test okay!");
  }while(0);

  tests++;
  printf("Test %d: Copy constructor\n",tests);
  do{
    mgrList<intNode> clist = ilist;
    if(clist.isEmpty()){
      puts("*** Error: copy is empty!");
      errors++;
      break;
    }
    if(ERR_NO_ERROR != ( res = clist.isValid())){
      printf("*** Error: copy is corrupt: 0x%x\n",res);
      errors++;
      break;
    }
    if(clist.count() != ilist.count()){
      printf("*** Error: sizes differ (o: %d -> c: %d)\n",
	     ilist.count(),clist.count());
      errors++;
      break;
    }
    puts("+++ Copy completed!");
    mgrListWalk<intNode> cwalk(&clist);
    cwalk.rewind();
    printf("???  copy:");
    while((in = cwalk())){
      in->dump();
    }
    putchar('\n');
  }while(0);


  tests++;
  printf("Test %d: Purge test on container class\n",tests);
  do{
    res = ilist.purge();
    if(res != ERR_NO_ERROR){
      printf("*** Error: purge() failed: 0x%.4x\n",res);
      errors++;
      break;
    }
    res = ilist.isValid();
    if(res != ERR_NO_ERROR){
      printf("*** Error: list corrupted after purge: 0x%.4x\n",res);
      errors++;
      break;
    }
    if(!ilist.isEmpty()){
      puts("*** Error: list not empty after purge!");
      errors++;
      break;
    }    
    puts("+++ purge() succeeded OK!");
  }while(0);

  printf("\n%d tests completed with %d errors.\n",tests,errors);
  printf("Used version: %s\n",ilist.VersionTag());

  return 0;
}
#endif // TEST
