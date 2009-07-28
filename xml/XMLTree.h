/*
 *
 * XML as Hierarchical Tree
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: XMLTree.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  XMLNode  - HTreeNode for XML
 *  XMLTree  - XTree of XMLNode
 *
 * This defines the values:
 *
 */

#ifndef _XML_XML_H_
# define _XML_XML_H_

#include <unistd.h>
#include <htree.h>
#include <expat.h>
#include <map>
#include <StringBuffer.h>
#include <StreamDump.h>

/*! \file XMLTree.h
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

namespace mgr {

  //! XML content representation
  /*! XMLNode either holds a tag with attributes
      stored in a vector of string pairs, or
      holds text contents. The type of node
      is determined by the isTag property.
      Non-Tags cannot hold child nodes. The tag
      name string is used to hold text contents in case
      of a text content node.

      XMLTree assumes it owns its nodes. When creating
      content to add to the tree make sure that the
      XMLNode is created with new!
  */
  class XMLNode : public HTreeNode {
    friend class XMLTree;
  public:
    /*! \brief Attribute=value pairs for the current tag

        The key value of the map denotes the attribute, which
	should comply with the xml syntax for attributes. The
	map value contains the value string, which is quoted
	on export.
    */
    typedef std::map<StringBuffer,StringBuffer> AttributeList;

  protected:
    bool isTag;                //!< if false, this is text content
    StringBuffer Tag;          //!< the tag name, or the text content, depending on isTag
    AttributeList attributes;  //!< list of tag attributes, empty for text content

  public:
    //! Standard CTOR creates a text node
    XMLNode() : isTag( false ) {}
    //! CTOR with tag name creates a tag node
    /*! \param tg Name of the tag
        \param itg Tag or text content

	Typical invocation is either XMLNode("my_tag") or
	XMLNode("This is some text.",false) in order to create
	a new tag or a text content node.
    */
    XMLNode(const char *tg, bool itg = true) : isTag(itg), Tag(tg) {}

    //! CTOR with tag name creates a tag node
    /*! \param tg Name of the tag
        \param itg Tag or text content

	Typical invocation is either XMLNode("my_tag") or
	XMLNode("This is some text.",false) in order to create
	a new tag or a text content node.
    */
    XMLNode(const StringBuffer& tg, bool itg = true) : isTag(itg), Tag(tg) {}

    //! Dump node to StreamDump
    m_error_t dump(StreamDump& s) const;

    //! Get Attribute list
    /*! \return The attributes map as const.

        \note The method does not check whether this is
	a tag or text content. In the latter case you're
	supposed to get an empty map.
    */
    const AttributeList &Attributes( void ) const {
      return attributes;
    }

    //! Get tag name or text contents
    /*! \retval s Copy of the Tag property
        \return true, if s is to be interpreted as tag

        If content() returns true, s holds a copy of
	the Tag name. Otherwise, the node is text content
	and s holds a copy of this content.

	\note StringBuffer is based on _wtBuffer. So the copy
	is just a reference until a write pointer is
	requested by either the XMLNode or your instance.
    */
    bool content( StringBuffer& s ) const {
      s = Tag;
      return isTag;
    }

    //! Set attribute list
    /*! \param l Attribute list to replace current attributes with
        \return Error code as defined in mgrError.h

	Replaces the current list of arguments with a new list.
	This discards the old list. Use addAttribute(), if you want
	to add single attributes.

	If the entry refers to text content, the attribute
	list will not be set and ERR_PARAM_TYP is returned.
    */
    m_error_t Attributes( const AttributeList& l ){
      if(!isTag) return ERR_PARAM_TYP;
      attributes = l;
      return ERR_NO_ERROR;
    }
    
    //! Add attribute to list of attributes
    m_error_t addAttribute( const char *id, const char *val);

    //! Add attribute to list of attributes
    m_error_t addAttribute( StringBuffer& sid, StringBuffer& val);    

    //! Make the node independent
    /*! \return Error code as defined in mgrError.h

        This is the _wtBuffer::branch() for the
	node. Calling branch() ensures that
	all data used is owned by the tree, can be written
	and will not vanish suddenly, e.g. due to destruction
	of some value, which is simply referred.

	If a value is already owned by the tree, it is not copied.
	All you lose is some processing time.
	
	Branching includes attribute values, if exist. It does not
	include attribute names, however.

	\warning The key StringBuffer in the AttributeList must be
	branched in advance or kept constant as reference 
	std::map has it as const!

	\todo It is only necessary to branch _wtBuffer instances,
	which are referrals. Maybe we shall optimize a little.
    */
    m_error_t branch( void ){
      m_error_t err = Tag.branch();
      if(ERR_NO_ERROR != err) return err;
      AttributeList::iterator it = attributes.begin();
      while(it != attributes.end()){
	// The key StringBuffer in the AttributeList must be
	// branched in advance or kept constant as reference
	// std::map has it as const!
	// it->first.branch();
	it->second.branch();
	++it;
      }
      return err;
    }
  };

  //! DOM like XML representation using HTree
  /*! This class manages XMLNode entries as a DOM
      for XML. It can be managed like any other
      HTree. Text content shall be added using
      addText() with the parental tag set
      as current(). Tag contents is managed as a
      sequence of tags and text contents. If new text
      is added, it is added to the end of the sequence.
      If the end of the sequence already is text content,
      the text is merged into the same entry.

      \todo Add support for <?lang ?> embedded languages
      and namespaces.
  */
  class XMLTree : public XTree<XMLNode> {
  protected:
    //! Get node to add text to
    XMLNode *mergeText( void );

  public:
    //! The XMLTree DTOR deallocates all nodes
    /*! \note This is different from the standard minimal behaviour of HTree */
    virtual ~XMLTree(){
      remove(sroot,true);
      XTree<XMLNode>::clear();      
    }

    //! Escape text contentsin order to be sane inside a XML document
    m_error_t escapeText( StringBuffer& txt ) const;

    //! Clear the tree and destroy all nodes
    virtual void clear( void );

    //! Add text content to the end of the sequence of the current tag's children
    m_error_t addText(const char *txt){
      XMLNode *tn = mergeText();
      if(!tn) return ERR_MEM_AVAIL;
      tn->Tag += txt;
      return ERR_NO_ERROR;
    }
    //! Add text content to the end of the sequence of the current tag's children
    m_error_t addText(const StringBuffer& txt){
      XMLNode *tn = mergeText();
      if(!tn) return ERR_MEM_AVAIL;
      tn->Tag += txt;
      return ERR_NO_ERROR;
    }

    //! Add text content to the end of the sequence of the current tag's children
    XMLTree &operator<<(const char *txt){
      addText(txt);
      return *this;
    }

    //! Add text content to the end of the sequence of the current tag's children
    XMLTree &operator<<(const StringBuffer& txt){
      addText(txt);
      return *this;
    }

    //! Version information string
    const char * VersionTag(void) const;

    //! Dump current sub-tree to StreamDump
    m_error_t dump(StreamDump& s) const;
  };

};
#endif // _XML_XML_H_
