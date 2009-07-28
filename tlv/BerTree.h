/*
 *
 * BER, DER, TLV, ASN.1, ... Trees
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: BerTree.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  BerTag  - node for a TAG
 *  BerTree - the XTree of berTag
 *
 * This defines the values:
 *
 */

#ifndef _TLV_BERTREE_H_
# define _TLV_BERTREE_H_

#include <mgrMeta.h>
#include <mgrError.h>
#include <htree.h>
#include <wtBuffer.h>
#include <StreamDump.h>
#include <stdio.h>

/*! \file BerTree.h
    \brief Implementation of BER coded Tag Length Value (TLV) structures based on HTree

    This is a generic implementation of BER coded data. It does not contain any
    semantics, but sticks to simple BER grammar. In order to implement semantics
    as e.g. in ASN.1 inherit from this class as storage structure.

    Each tag is represented as BerTag class, which inherits HTreeNode and constitutes
    the basic object in the BerTree, which inherits XTree<BerTag>. As such it is a
    standard HTree implementation.

    BerTag inherits HTreeNode and contains 3 instances _wtBuffer holding Tag, Length, and value each.
    Actually, _wtBuffer is inherited into BerContentRegion as a container for binary
    data. BerContentLength and BerContentTag inherit from BerContentRegion in order to
    provide the semantics constituting the BER grammar.

    \author Dr. Lars Hanke
    \date 2006-2007
*/

namespace mgr {

  /*! \class BerContentRegion
      \brief Buffer to hold binary data

      This essentially is a wtBuffer<unsigned char> with one distinct
      deviation. While _wtBuffer can be invalid, BerContentRegion can only
      be empty. This required redefining the comparison operators, since
      two empty regions are equal, two undefined _wtBuffer are not.

      Extending this, regions can have a dummyLength(). This means they
      keep the space for some data, but do not hold the data themselves,
      i.e. do not allocate or reference any memory. This is used to implement
      compound tags, where the real contents are stored to other BerTag
      nodes in the child sequence of the current tag node in BerTree.

      For convenience assignment to a region is possible from all
      _wtBuffer instances, instead of any typed variant. Note that
      the results are not portable in general, but BerContentRegion
      is \b binary data after all.

      In order to faciliate debugging a dump() method using StreamDump
      is added. Consider the HexDump class to view binary data.

      \todo Chunk size management is still bad up to useless.
  */
class BerContentRegion : public wtBuffer<unsigned char> {
protected:
  enum {
    DEFAULT_CHUNK = 128  //!< Default chunk size for _wtBuffer
  };

public:
  //! Standard CTOR producing empty region
  /*! \param chunk_size Allocation chunk size to use with _wtBuffer */
  BerContentRegion(const size_t& chunk_size = DEFAULT_CHUNK) : 
    wtBuffer<unsigned char>(chunk_size) {}

  //! Copy CTOR
  BerContentRegion(const BerContentRegion& r) : wtBuffer<unsigned char>(r) {}

  //! Assignment Operator type wrapping
  inline BerContentRegion& operator=(const BerContentRegion& r){
    wtBuffer<unsigned char>::operator=(r);
    return *this;
  }

  //! Assignment Operator from any _wtBuffer
  inline BerContentRegion& operator=(const _wtBuffer& r){
    // this is okay, since records are kept in wtBuffer<>
    _wtBuffer::operator=(r);
    return *this;
  }

  //! Set a dummy length to this region
  /*! \param l Length of region

      Dummy length is indicated by no reference to neither
      a fixed referral nor a __wtBuffer, but a non-zero length.
      Actually, zero length is a special dummy length indicating
      an empty container, which is different from an undefined
      container in _wtBuffer.

      Any previous contents of the region is discarded.
  */
  inline void dummyLength(const size_t l){
    free();
    buf.fix = NULL;
    length = l;
  }

  //! Write region contents to a StreamDump
  /*! \param s StreamDump to write to
      \return Error code as defined in mgrError.h

      This is handy to both produce debugging output
      e.g. using HexDump or write the BerTree to
      a file using e.g. FileDump.

      \note Dummy lengths will return ERR_PARAM_NULL.
  */
  inline m_error_t write(StreamDump& s) const {
    if(!length) return ERR_NO_ERROR;
    if(!get_fix()) return ERR_PARAM_NULL;
    size_t tl = length;
    return s.write(get_fix(),&tl);
  }

  //! Equality operator
  /*! \param r Region to compare
      \return true if region contents are equal

      Regions are equal, if they either contain the same data of the
      same length, or if they are dummyLength() regions of the
      same length.

      \todo Check whether it is desirable to have equality
      for dummy lengths.
      \todo The function has grown, consider to put it into
      BerTree.cpp.
  */
  inline bool operator==(const BerContentRegion& r) const {
    if(length != r.length) return false;
    if(!readPtr() && !r.readPtr()) return true;
    if(!readPtr() || !r.readPtr()) return false;
    if(memcmp(readPtr(),r.readPtr(),length)) return false;
    return true;
  }

  //! Inequality operator
  /*! \param r Region to compare
      \return true if region contents are not-equal

      This is just the logical not of the equality operator
      operator==(const BerContentRegion& r).
  */
  inline bool operator!=(const BerContentRegion& r) const {
    return !operator==(r);
  }

  //! Dump node contents to a c-string
  ssize_t dump(char *s, const size_t& l) const;

  //! Has been some version of branch()
  /*! \deprecated Do not use.
      \todo Check removal from sources.
  */
  m_error_t own(_wtBuffer& b);
};

  /*! \class BerContentTag
      \brief BerContentRegion holding the Tag value itself

      This class represent the region T in a TLV BER structure.
      Apart from being a BerContentRegion binary container,
      it implements the BER tag semantics and makes it accessible
      to the application.

      \dot
      digraph tag_field {        
        byte [shape=record,label="{{{<c> c|{7|6}}|{<t> t|5}|{<v> number|{4|3|2|1|0}}}|First Tag Byte}"];
	Class -> byte:c;
	Type -> byte:t;
	Number -> byte:v;
      }
      \enddot

      Using any kind of initialisation the appropriate semantics
      is applied. This is achieved by overloading replace(). You can
      still assign strange values to a tag by assigning from a 
      _wtBuffer. This will allow to create tags like '5f 01'. Synthesis
      using replace() would yield '41' both indication the application
      specific primitive tag number 1.
      
      Tags do not support insert() or prepend(). Nor do they support
      a dummyLength().

      \todo Should neither support append(). Check why it is not excluded.
      \todo Supply synthesis function for EMV style '1f 01' tags.
  */
class BerContentTag : public BerContentRegion {
protected:
  enum {
    DEFAULT_CHUNK = 8   //!< chunk size for tag regions
  };

public:
  //! Tag grammar type
  enum ber_type {
    BER_PRIMITIVE = 0,       //!< Tag without sub-tags
    BER_CONSTRUCTED = 1      //!< Container tag
  };
  
  //! Tag semantics class
  enum ber_class {
    BER_UNIVERSAL = 0,       //!< Universal standard tag, e.g. ASN.1
    BER_APPLICATION = 1,     //!< Application standardized, e.g. EMV
    BER_CONTEXT = 2,         //!< Context specific, e.g. inside a template
    BER_PRIVATE = 3          //!< Private namespace
  };
  
  //! Tag grammar type
  typedef enum ber_type BerTagType;
  //! Tag semantics class
  typedef enum ber_class BerTagClass;   

#include "BerTree-meta.h"

public:
  //! Standard CTOR creating undefined tag
  BerContentTag(const size_t& chunk_size = DEFAULT_CHUNK) : 
    BerContentRegion(chunk_size) {}
  
  //! CTOR initialising from memory
  /*! \param d Start address of tag
      \param l Length of buffer
      
      Reads the tag from the given loaction according
      to the BER tag number coding. If the tag is incorrectly
      coded, i.e. replace() produces an error, the CTOR will
      throw.
  */
  BerContentTag(const unsigned char * const d, const size_t & l) {
    m_error_t error = replace(d,l);
    if(error != ERR_NO_ERROR) 
      mgrThrowFormat(error,"Parsing BER from %p (%d) in %p",d,l,this);
  }

  //! CTOR initialising by synthesis
  /*! \param t Tag number without any class or type qualifiers
      \param ty Grammatical type of tag (Type)
      \param cl Semantic context of tag (Class)
      
      Synthesizes the tag using the shortest possible
      representation. If the tag cannot be synthesized, e.g.
      due to memore shortage, the CTOR will throw.
  */  
  BerContentTag(const size_t& number, const BerTagType& ty, const BerTagClass& cl){
    m_error_t error = replace(number,ty,cl);
    if(error != ERR_NO_ERROR)
      mgrThrowExplain(error, "Creating BER tag");
  }

  //! Copy CTOR
  BerContentTag(const BerContentTag& t) : BerContentRegion(t){}

  //! Assignment operator (type wrapper)
  BerContentTag& operator=(const BerContentTag& t){
    BerContentRegion::operator=(t);
    return *this;
  }

  //! Read tag from BER coded memory
  /*! \param d Start of tag in memory
      \param l Valid buffer length
      \return Error code as specified in mgrError.h

      Reads a tag according to BER grammar. If the tag
      cannot be concluded within the valid buffer
      ERR_PARAM_RANG is returned.

      The contents read for the tag are referenced
      by the underlying _wtBuffer. In order to obtain the
      actual length in bytes used for the tag, query
      the byte_size() method of the BerTag.
  */
  m_error_t replace(const unsigned char * const d, const size_t & l);

  //! Synthesize tag from type, class and number
  m_error_t replace(const size_t& number, const BerTagType& ty, const BerTagClass& cl);

  //! Return the grammatical type of the tag
  BerTagType  Type(void) const;

  //! Return the semantic context of the tag (class)
  BerTagClass Class(void) const;    

  //! prepend() blocked
  void prepend(void){}
  //! insert() blocked
  void insert(void){}
  //! dummyLength() blocked
  void dummyLength(void){}
};

  /*! \class BerContentLength
      \brief BerContentRegion holding the Length value itself

      This class represent the region L in a TLV BER structure.
      Apart from being a BerContentRegion binary container,
      it implements the BER length coding and makes it accessible
      to the application.

      Using any kind of initialisation the appropriate semantics
      is applied. This is achieved by overloading replace(). You can
      still assign strange values to a tag by assigning from a 
      _wtBuffer. This will allow to create tags like '5f 01'. Synthesis
      using replace() would yield '41' both indication the application
      specific primitive tag number 1.
      
      Tags do not support insert() or prepend(). Nor do they support
      a dummyLength().

      \todo Should neither support append(). Check why it is not excluded.
      \todo Supply synthesis function for EMV style '1f 01' tags.
  */
class BerContentLength : public BerContentRegion {
protected:
  size_t  val;            //!< Numerical value of the length
                          /*!< Length ~0 is invalid */
  enum {
    DEFAULT_CHUNK = 8     //!< Default chunk size for _wtBuffer
  };

public:
  //! Standard CTOR producing invalid length
  BerContentLength(const size_t& chunk_size = DEFAULT_CHUNK) :
    BerContentRegion(chunk_size){
    val = (size_t)~0;
  }

  //! CTOR initialising from memory
  /*! \param d Start length field
      \param l Length of buffer
      
      Reads the length from the given loaction according
      to the BER length coding. If the length coding exceeds
      the buffer length or the maximum value of size_t, i.e. 
      replace() produces an error, the CTOR will
      throw.
  */
  BerContentLength(const unsigned char * const d, const size_t & l){
    m_error_t error = replace(d,l);
    if(error != ERR_NO_ERROR)
      mgrThrowFormat(error,"Parsing BER from %p (%d) in %p",d,l,this);
  }

  //! Copy CTOR
  BerContentLength(const BerContentLength& t) : BerContentRegion(t){}

  //! Assignment operator
  BerContentLength& operator=(const BerContentLength& t){
    BerContentRegion::operator=(t);
    val = t.val;
    return *this;
  }

  //! Equality operator
  /*! \param l Length to compare with
      \return true is length is equal

      The operator only checks the length value. Two undefined
      BerContentLength instances are not equal.
  */
  inline bool operator==(const BerContentLength& l) const {
    if(val == (size_t)~0) return false;
    return (val == l.val);
  }
  
  //! Get length value
  /*! \return Value of the length element; ~0 relates to undefined */
  inline const size_t &value(void) const { return val; }

  //! Set length value
  m_error_t value(const size_t& l);

  //! Overloaded BerContentRegion::free()
  /*! This function also clears the value. */
  inline void free(void){
    val = ~0;
    BerContentRegion::free();
  }

  //! Replacement initialising from memory
  /*! \param d Start address of length field
      \param l Length of buffer
      \return Error code as defined in mgrError.h
  */
  m_error_t replace(const unsigned char * const d, const size_t & l);

  //! Replacement synthesizing length
  /*! \param l length to set
      \return Error code as defined in mgrError.h

      This is just a wrapper for value( const size_t& l).
  */      
  inline m_error_t replace(const size_t& l) { return value(l); }
  
  //! prepend() blocked
  void prepend(void){}
  //! insert() blocked
  void insert(void){}
  //! dummyLength() blocked
  void dummyLength(void){}
};

  /*! \class BerTag
      \brief An entity of BER coded data as node of HTree

      This class combines BerContentTag, BerContentLength, and
      BerContentRegion to a TLV structure forming a BER
      coded entity.
  */
class BerTag : public HTreeNode {
protected:
  class BerContentTag Tag;         //!< The Tag
  class BerContentLength Length;   //!< The Length
  class BerContentRegion Value;    //!< The Value
    
public:    
  //! CTOR for empty BER structure
  BerTag() : HTreeNode() {}

  //! CTOR reading from BER coded memory
  /*! \param d Start of BER coded memory
      \param l length of BER coded memory
 
      Creates and initializes a BerTag from BER coded
      memory. If the memory contains only a tag field, i.e.
      the buffer ends exactly before the length field,
      the tag is created empty.

      All other errors like buffer underrun or CTOR errors
      of the sub entities throw.
  */
  BerTag(const unsigned char * const d, const size_t & l){
    m_error_t error = readBer(d,l);
    // ERR_CANCEL is thrown, if the buffer contains only a valid tag, 
    // i.e. no length nor value
    if((error != ERR_NO_ERROR) && (error != ERR_CANCEL))       
      mgrThrowFormat(error,"Parsing BER from %p (%d) in %p",d,l,this);
    if(error == ERR_CANCEL) {
      Length.value(0);
      if(Tag.Type() == BerContentTag::BER_CONSTRUCTED){
	Value.dummyLength(0);
      } else {
	Value.trunc(0);
      }
    }
  }

  //! CTOR reading from BER coded value
  /*! \param r BerContentRegion holding BER coded data
 
      Creates and initializes a BerTag from a BER coded
      buffer. This is the standard method to parse
      composite tags. Tags are expected to be complete,
      i.e. T only fields are not accepted contrary
      to the CTOR from memory.

      All errors like buffer underrun or CTOR errors
      of the sub entities throw.
  */
  BerTag(const BerContentRegion& r) : HTreeNode() {
    m_error_t error = readBer(r.readPtr(), r.byte_size());
    if(error != ERR_NO_ERROR) 
      mgrThrowExplain(error,"Parsing BER from region");	
  }

  //! CTOR from Tag only structure
  /*! \param t BerContentTag structure with the tag to create

      Creates a full BerTag from a tag given as
      BerContentTag. The tag is copied, i.e. you may discard
      the original. Take care to branch() the buffer, if you discard
      original data, which are only referenced by the _wtBuffer.

      If the tag is primitive it is created as empty tag.
      Compound tags are created as length 0 dummies.
  */
  BerTag(const BerContentTag& t) : HTreeNode() {
    Tag = t;
    Length.value(0);
    if(Tag.Type() == BerContentTag::BER_CONSTRUCTED){
      Value.dummyLength(0);
    } else {
      Value.trunc(0);
    }
  }

  /*! \copydoc BerContentTag::BerContentTag(const size_t&, BerTagType&, BerTagClass&)
      This CTOR creates the entire BerTag.
      If the tag is primitive it is created as empty tag.
      Compound tags are created as length 0 dummies.
  */
  BerTag(const size_t& number, 
	 const BerContentTag::BerTagType& ty, 
	 const BerContentTag::BerTagClass& cl) : HTreeNode() {
    Tag.replace(number,ty,cl);
    Length.value(0);
    if(Tag.Type() == BerContentTag::BER_CONSTRUCTED){
      Value.dummyLength(0);
    } else {
      Value.trunc(0);
    }
  }

  //! Copy CTOR
  BerTag(const BerTag& t) : HTreeNode(t) {
    Tag = t.Tag;
    Length = t.Length;
    Value = t.Value;
  }

  //! Assignment operator
  BerTag& operator=(const BerTag& t){
    Tag = t.Tag;
    Length = t.Length;
    Value = t.Value;

    return *this;
  }

  //! Equality operator
  bool operator==(const BerTag& t){
    return ( (Tag == t.Tag) &&
	     (Length == t.Length) &&
	     (Value == t.Value));
  } 
			
  //! Parse BER coded data from memory
  m_error_t readBer(const unsigned char * const d, const size_t & l);

  //! Total size
  /*! \return Full length of TLV structure in octets if dumped */
  inline size_t size(void) const {
    return Tag.byte_size() + Length.byte_size() + Value.byte_size();
  }
  
  //! Content size
  /*! \return Length of content as stored in the length field
      \note This may in general differ from the actual
      length BerContentRegion::byte_size(). Using BerTag as
      the only interface for constructing BerTag instances
      should avoid this.
  */
  inline size_t c_size(void) const {
    return Length.value();
  }
  
  //! Get the content Buffer
  /*! \return Constant reference to content buffer */
  inline const class BerContentRegion &content(void) const {
    return Value;
  }
  
  //! Set content from memory
  /*! \param d Start of content ion memory
      \param l Size of content in octets
      \return Error code as defined in mgr Error.h

      Sets the new contents into the value field and
      updates the length field for the new contents.
  */
  inline m_error_t content(const unsigned char * const d, const size_t& l) {
    Value.replace(d,l);
    return Length.replace(Value.byte_size());
  }

  //! Set a dummy length content
  /*! \param l Length to set
      \return Error code as defined in mgr Error.h

      Discards any contents of value and updates value and
      length fields according to the length given. 

      \note This method does not check, whether the tag
      is compound. Dummy length makes no sense for
      primitive tags.
      \todo Maybe primitive tags should be rejected.
  */
  inline m_error_t content(const size_t& l) {
    Value.dummyLength(l);
    return Length.replace(l);
  }

  //! Allocate a content buffer
  /*! \param l Length to allocate for content buffer
      \return Non-constant pointer to content buffer

      Allocation is performed by _wtBuffer::trunc(), i.e.
      preserving any previous contents. Also, the length 
      field is kept current.

      If any of the changes fail a NULL pointer is returned.
      The detailed error code is not available.
  */
  inline unsigned char *allocate(const size_t& l){
    if(ERR_NO_ERROR != Value.trunc(l)){
      Length.replace(0);
      return NULL;
    }
    if(ERR_NO_ERROR != Length.replace(l)){
      Value.trunc(0);
      return NULL;
    }
    return Value.writePtr();
  }

  //! Initialize content from a _wtBuffer
  /*! \param b Buffer to read content data from
      \return Error code as defined in mgrError.h

      Assigns the contents of b to the content field
      and updates the length field accordingly.
  */
  inline m_error_t content(const _wtBuffer& b){
    m_error_t res = ERR_NO_ERROR;
    Value.operator=(b);
    if(res != ERR_NO_ERROR){
      Length.replace(0);
      return res;
    }
    return Length.replace(Value.byte_size());
  }
  
  //! Get the tag
  /*! \return Constant reference to the BerContentTag */
  inline const class BerContentTag &tag(void) const {
    return Tag;
  }
  
  //! Read Tag field from memory
  /*! \param d Start of tag in memory
      \param l length of buffer holding tag
      \return Error code as defined in mgrError.h

      Replaces the tag field with the tag read from memory.
      The function also updates the length field from the actual
      content length. The latter of course is not related with
      replacing the tag, but keeps the BerTag sane.
  */
  inline m_error_t tag(const unsigned char * const d, const size_t & l){
    m_error_t err = Tag.replace(d,l);
    if(err != ERR_NO_ERROR) return err;
    return Length.replace(Value.byte_size());
  }
  
  //! Synthesize Tag field
  /*! \param t Tag number without any class or type qualifiers
      \param ty Grammatical type of tag (Type)
      \param cl Semantic context of tag (Class)
      \return Error code as defined in mgrError.h

      Replaces the tag field with the tag as sythesized.
      The function also updates the length field from the actual
      content length. The latter of course is not related with
      replacing the tag, but keeps the BerTag sane.

      \sa BerContentTag::replace(const size_t&, const BerTagType&, const BerTagClass& cl)
  */
  inline m_error_t tag(const size_t& number, const BerContentTag::BerTagType& ty, const BerContentTag::BerTagClass& cl){
    m_error_t err = Tag.replace(number, ty, cl);
    if(err != ERR_NO_ERROR) return err;
    return Length.replace(Value.byte_size());
  }
  
  //! Sanitize bookkeeping optionally including child nodes
  m_error_t recalcSize(size_t &s, const bool follow = false);

  //! Dump sub-tree to file
  ssize_t dump(FILE *f = stdout, const char *prefix = NULL) const;

  //! Write sub-tree to StreamDump
  m_error_t write(StreamDump& s) const;

  //! Discard node
  void clear(void);

  //! Enforce own copies of all _wtBuffer instances
  /*! \return Error code as defined in mgrError.h

      This function performs a _wtBuffer::branch() on all 
      three fields of the BerTag. After that it is ensured that
      all external initialisation data can be safely discarded.
  */
  inline m_error_t detach(void){
    m_error_t res = Value.branch();
    if(res != ERR_NO_ERROR) return res;
    res = Tag.branch();
    if(res != ERR_NO_ERROR) return res;
    return Length.branch();
  }

  //! Version information string
  const char *VersionTag(void) const;
};

  /*! \class BerTree
      \brief BER coded data as HTree of BerTag

      BerTree can be the overall owner of nodes. This will
      cause node deletion on removal or discarding the
      tree. It can also be just a maintenance struture,
      called an iterator. In the latter case no physical
      deletion of nodes is performed. These modes are
      determined by the property ownNodes.
  */
class BerTree : public XTree<class BerTag> {
protected:
  wtBuffer<unsigned char> input;         //!< Buffer for input data during parsing
  //wtBuffer<unsigned char> output;
  const unsigned char *garbage;          //!< pointer to unparsed trailing data
  bool ownNodes;                         //!< flag to indicate whether the nodes can be deleted by the Tree, i.e. are owned

public:
  //! Standard CTOR for empty tree and buffer
  BerTree() : XTree<class BerTag>(), garbage( NULL ), ownNodes( false ) {}

  //! CTOR parsing BER from memory
  /*! \param data Start of memory region containing BER data
      \param l Length of data to parse

      This CTOR calls replace() to read the data from memory. Invalid
      trailing data is attached to garbage. The _wtBuffers created
      are referrals, i.e. the memory consumption relates to
      the management information only.
  */
  BerTree(const unsigned char *data, size_t l) : XTree<class BerTag>() {
    garbage = NULL;
    replace(data,l);
  }

  //! CTOR initalising from linked BerTag structure
  /*! \param n Root node of tree

      Creates the management structure for the tree of linked
      BerTag nodes. This enables e.g. to have a secondary
      BerTree maintaining a sub-tree.

      \warning If you use more than one BerTree on the
      same structure the paths are not synchronized. This may cause
      failure, if you delete a node in the path of one tree
      using methods of the other.
  */
  BerTree(BerTag *n) : XTree<class BerTag>(n), garbage( NULL ), ownNodes( false ) {}

  //! Deep copy a BerTree
  inline m_error_t clone(const BerTree& t){
    // FIXME: clone garbage
    garbage = NULL;
    ownNodes = true;
    return XTree<class BerTag>::clone(t);
  }

  //! Standard DTOR
  virtual ~BerTree();

  //! Clear and optionally destroy
  virtual void clear(void);

  //! copy CTOR
  BerTree(const BerTree& t) : XTree<class BerTag>(t) {
    garbage = t.garbage;
    ownNodes = false;
  }
  
  //! Assigment operator
  BerTree& operator=(const BerTree& t);

  //! Check the tree for iterator
  /*! \return true, if the nodes are only maintained rather than owned

      Iterator BerTree structures will not delete nodes
      when nodes are discarded or the entire tree runs
      out of scope. This method allows to check the type
      of the BerTree.
  */
  inline bool iteratorOnly(void) const { return !ownNodes; }

  //! Declare tree an iterator
  /*! \param disown If true, the BerTree is an iterator and will not delete nodes
    
      Iterator BerTree structures will not delete nodes
      when nodes are discarded or the entire tree runs
      out of scope. This method allows to check the type
      of the BerTree.
  */     
  inline void iteratorOnly(bool disown) { ownNodes = !disown; }

  //! BER memory parser
  m_error_t replace(const unsigned char *data, size_t l, bool copy = true);

  //! BER parser for contents of (nested) tags
  m_error_t parseContent(BerTag *c) const;

  //! Deletion of BerTag
  virtual void freeNode(HTreeNode *n) const;

  //! Return unparsed data trailing the parsed contents
  inline const unsigned char *trailer(void) const {
    return garbage;
  }

  //! Dump BerTree to file
  ssize_t dump(FILE *f = stdout, const char *prefix = NULL);

  //! Dump BerTree to StreamDump
  m_error_t write(StreamDump& s, bool calc = true);

  //! Recalculate sizes of compound nodes
  inline m_error_t sanitize(void){
    if(!sroot) return ERR_NO_ERROR;
    size_t s = 0;
    return static_cast<BerTag *>(sroot)->recalcSize(s,true);
  }

  //! Calculate size of entire BerTree data if dumped
  inline size_t fullSize(void) const {
    size_t s = 0;
    BerTag *c = static_cast<BerTag *>(sroot);

    while(c){
      s += c->size();
      c = static_cast<BerTag *>(c->getNext());
    }

    return s;
  }

  //! Find a specific Tag
  BerTag *find(const BerContentTag& t) const;
  //! Find a specific Tag
  BerTag *find(const unsigned char *data, const size_t& s, m_error_t *err = NULL) const ;

};

};

#endif // _TLV_BERTREE_H_
