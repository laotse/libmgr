/*
 *
 * XML as Hierarchical Tree
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: XMLTree.cpp,v 1.6 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  XMLNode  - HTreeNode for XML
 *  XMLTree  - XTree of XMLNode
 *
 * This defines the values:
 *
 */

#include "XMLTree.h"
#include "XMLTree.tag"

/*! \file XMLTree.cpp
    \brief XML content mapped to HTree

    The XMLTree class represents XML data as hierarchical tree,
    i.e. implements somewhat like a DOM representation.

    Implementation follows general HTree implementation.
    XMLNode defines the node contents, which is either 
    a tag with attributes or text contents. The XMLTree
    inherits from the XTree template using XMLNode as
    template argument.

    \todo Add support for embedded language even
    xml itself, i.e. <?lang ?> clauses; and namespaces.

    \author Dr. Lars Hanke
    \date 2006-2007
*/

//#define DEBUG
//! Debug allocation espacially inside _wtBuffer
#define DEBUG_ALLOCATE 1
//! Debug tree operations
#define DEBUG_TREE     2
#define DEBUG (DEBUG_TREE)
#include <mgrDebug.h>

#ifdef DEBUG
# include <stdio.h>
#endif

using namespace mgr;

static m_error_t print_attrib(StreamDump& s,
			      const StringBuffer& a, 
			      const StringBuffer& v){
  m_error_t ierr = s.putchar(" ");
  if(ERR_NO_ERROR != ierr) return ierr;
  size_t len = a.strlen();
  ierr = s.write(a.readPtr(),&len);
  if(ERR_NO_ERROR != ierr) return ierr;
  len = 2;
  ierr = s.write("=\"",&len);
  if(ERR_NO_ERROR != ierr) return ierr;
  len = v.strlen();
  ierr = s.write(v.readPtr(),&len);
  if(ERR_NO_ERROR != ierr) return ierr;
  return s.putchar("\"");
}

m_error_t XMLNode::dump(StreamDump& s) const {
  if(Tag.strlen() == 0) return ERR_NO_ERROR;
  size_t len = Tag.strlen();
  if(!isTag)
    return s.write(Tag.readPtr(),&len);
  m_error_t ierr = s.putchar("<");
  if(ERR_NO_ERROR != ierr) return ierr;
  ierr = s.write(Tag.readPtr(),&len);
  if(ERR_NO_ERROR != ierr) return ierr;
  for(AttributeList::const_iterator catt = attributes.begin(); 
      catt != attributes.end(); ++catt){
    ierr = print_attrib(s,catt->first,catt->second);
    if(ERR_NO_ERROR != ierr) return ierr;
  }
  if(child){
    m_error_t ierr = s.putchar(">");
    if(ERR_NO_ERROR != ierr) return ierr;
    const XMLNode *cn = static_cast<XMLNode *>(child);
    while(cn){
      ierr = cn->dump(s);
      if(ERR_NO_ERROR != ierr) return ierr;
      cn = static_cast<XMLNode *>(cn->next);
    }
    len = 2;
    ierr = s.write("</",&len);
    if(ERR_NO_ERROR != ierr) return ierr;
    len = Tag.strlen();
    ierr = s.write(Tag.readPtr(),&len);
    if(ERR_NO_ERROR != ierr) return ierr;
    ierr = s.putchar(">");
    if(ERR_NO_ERROR != ierr) return ierr;	
  } else {
    len = 3;
    ierr = s.write(" />",&len);
    if(ERR_NO_ERROR != ierr) return ierr;
  }
  return ERR_NO_ERROR;
}

/*! \param id c-string of attribute name
    \param val c-string of attribute value
    \return Error code as defined in mgrError.h

    Adds the id="val" pair to the list of attributes.
    If the node is text content, ERR_PARAM_TYP is returned
    and nothing is added.

    If val is NULL the empty string is added as value.
*/
m_error_t XMLNode::addAttribute( const char *id, const char *val){
  if(!isTag) return ERR_PARAM_TYP;
  if(!id) return ERR_PARAM_NULL;
  if(!val) val = "";
  StringBuffer sid(id);
  StringBuffer sval(val);
  m_error_t err = sid.branch();
  if(err != ERR_NO_ERROR) return err;
  err = sval.branch();
  if(err != ERR_NO_ERROR) return err;
  attributes[sid] = sval;
  return ERR_NO_ERROR;
}

/*! \param id StringBuffer of attribute name
    \param val StringBuffer of attribute value
    \return Error code as defined in mgrError.h

    Adds the id="val" pair to the list of attributes.
    If the node is text content, ERR_PARAM_TYP is returned
    and nothing is added.

    If val is NULL the empty string is added as value.
*/
m_error_t XMLNode::addAttribute( StringBuffer& sid, StringBuffer& sval){
  if(!isTag) return ERR_PARAM_TYP;
  m_error_t err = sid.branch();
  if(err != ERR_NO_ERROR) return err;
  err = sval.branch();
  if(err != ERR_NO_ERROR) return err;
  attributes[sid] = sval;
  return ERR_NO_ERROR;
}

/*! If the current node has children, move to the end of the sequence of children and
    create a text content node, unless the final node already is text content.  
    If no children relate to the current node, create a new child sequence, if the node is a tag.
    If the node is text content, return the node itself.
*/
XMLNode *XMLTree::mergeText( void ){
  XMLNode *tn = static_cast<XMLNode *>(current()->child);
  if(tn) while(tn->next) tn = static_cast<XMLNode *>(tn->next);
  if(!tn || tn->isTag){
    tn = new XMLNode;
    appendChild(tn);
  }
  return tn;
}

m_error_t XMLTree::dump(StreamDump& s) const {
  XMLNode *n = current();
  if(!n) return ERR_PARAM_NULL;
  m_error_t ierr = n->dump(s);
  while((ierr == ERR_NO_ERROR) && (n->next)){
    n = static_cast<XMLNode *>(n->next);
    ierr = n->dump(s);
  }
  return ierr;
}

/*! Since a XMLTree owns all of its nodes, clear can safely discard them. */
void XMLTree::clear( void ){
  remove(sroot, true);
  // This is identical to HTree::clear() but more correct
  XTree<XMLNode>::clear();
}

m_error_t XMLTree::escapeText( StringBuffer& txt ) const {
  const char *rt = NULL;
  for(size_t s=0; s < txt.strlen(); ++s){
    switch(txt[s]){
    default:
      break;
    case '&':
      rt = "&amp;";
      break;
    case '<':
      rt = "&lt;";
      break;
    case '>':
      rt = "&gt;";
      break;
    }
    if( rt ){
      m_error_t ierr = txt.insert(s,1,rt,strlen(rt));
      if(ierr != ERR_NO_ERROR) return ierr;
      s+= strlen(rt) - 1;
      rt = NULL;
    }
  }
  return ERR_NO_ERROR;
}

const char * XMLTree::VersionTag(void) const{
  return _VERSION_;
}


/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <stdio.h>

void debugTree( XMLTree *tree ){
  if(!tree){
    fprintf(stderr,"Tree pointer is NULL!\n");
    return;
  }
  FileDump eout(stderr);
  XMLTree::Bookmark ref = tree->bookmark();
  StringBuffer sb;
  XMLNode *cn = tree->current();
  XMLNode *n = tree->root();
  int depth = 1;
  size_t written = 0;
  while(n){
    eout.printf( &written, "%.02d:",depth);
    if( n->content( sb ) ){
      // this is a tag
      eout.putchar('t');
    } else {
      // this is content
      eout.putchar('c');
    }
    eout.putchar(':');
    written = sb.strlen();
    eout.write(sb.readPtr(), &written);
    eout.putchar('\n');

    n = tree->iterate( &depth );
  }
  
  m_error_t res = tree->bookmark( ref );
  if((res != ERR_NO_ERROR) || (cn != tree->current())){
    fprintf(stderr,"Oops - bookmark error: was %p is %p (0x%.3x)\n",cn,tree->current(),(int)res);
  } else {
    fprintf(stderr,"+++ XMLTree.bookmark() test succeeded!\n");
  }
}

int main(int argc, char *argv[]){
  m_error_t res;
  size_t tests, errors;
  FileDump dout(stdout);

  tests = errors = 0;

  try {
    printf("Test %d: XMLTree.CTOR\n",++tests);
    XMLTree xml;
    puts("+++ XMLTree.CTOR() finished OK!");
    printf("Test %d: XMLNode.CTOR\n",++tests);
    XMLNode *xentry = new XMLNode("html");
    puts("+++ XMLNode.CTOR() finished OK!");
    printf("Test %d: XMLNode.dump()\n",++tests);
    res = xentry->dump(dout);
    dout.putchar('\n');
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: dump() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ dump() finished OK!");
    }
    xml.appendNext(xentry);
    xml.root();
    printf("Test %d: XMLTree.dump()\n",++tests);
    res = xml.dump(dout);
    dout.putchar('\n');
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: dump() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ dump() finished OK!");
    }
    xentry = new XMLNode("head");
    xml.appendChild(xentry,true);
    xentry = new XMLNode("title");
    xml.appendChild(xentry);
    xentry = new XMLNode("meta");
    xml.appendChild(xentry);
    xml.parent();
    xentry = new XMLNode("body");
    xml.appendChild(xentry,true);
    xentry = new XMLNode("p");
    xentry->addAttribute("align","center");
    xml.appendChild(xentry,true);
    xml << "Das ist ein Test-Absatz.";
    xml << " Mit zweimal Text!";
    xentry = new XMLNode("b");
    xml.appendChild(xentry,true);
    xml << "Fettschrift";
    xml.parent();
    xml << "Und einem Trailer!";
    debugTree( &xml );

    xml.root();
    printf("Test %d: XMLTree.dump()\n",++tests);
    res = xml.dump(dout);
    dout.putchar('\n');
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: dump() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ dump() finished OK!");
    }
    printf("Test %d: XMLTree.clear()\n",++tests);
    xml.clear();
    puts("+++ clear() finished OK!");

    printf("Test %d: XMLTree.escapeText()\n",++tests);
    StringBuffer txt("Das ist ein Test mit & ohne <Tags>.");
    res = xml.escapeText( txt );
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: escapeText() failed 0x%.4x\n",(int)res);
    } else {
      size_t l = 4;
      dout.write("??? ",&l);
      l = txt.strlen();
      dout.write(txt.readPtr(),&l);
      dout.putchar('\n');
      puts("+++ escapeText() finished OK!");
    }

    printf("\n%d tests completed with %d errors.\n",tests,errors);
    printf("Used version: %s\n",xml.VersionTag());
  }
  catch(const std::exception& e){
    printf("*** Exception thrown: %s\n",e.what());
  }
  catch(int err){
    printf("*** Exception thrown: 0x%.4x\n",err);
  }

  return 0;  
}

#endif //TEST
