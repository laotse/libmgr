/*
 *
 * A general framework for handling addressable regions
 *
 * (c) 2009 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: wtBuffer.h,v 1.4 2007/06/01 21:25:21 mgr Exp $
 *
 * This defines the classes:
 *  Addressable - An object with a start pointer and a length
 *                in octets. The class stores these coordinates
 *                and features range checking
 *
 * This defines the values:
 *
 */

#ifndef _UTIL_MEMORY_H_
# define _UTIL_MEMORY_H_

# include <mgr/meta.h>

/*! \file Addressable.h
    \brief A general framework for handling addressable regions

    \author Dr. Lars Hanke
    \date 2009    
*/

namespace mgr {

/*! \class Addressable
    \brief A not necessarily octet addressable region

    Addressable objects have a start address, i.e. pointer, and a
    length. Beyond simply storing these properties, the Addressable
    class features functions for range checking.

    A core goal for this class is to define efficient and correct
    range checking for pointer arithmetic. And this too is the main
    reason for using it.

    Such objects are linear, i.e. there are no gaps in the address
    space, addresses are ordered and allow at least random read access.
*/
class Addressable {
protected:
  const void *Region;  //!< The start address of the Addressable region
  size_t Length;       //!< The number of octets in the Addressable region

public:
  /*! \brief The type for octets 
   */
  typedef unsigned char byte;

  /*! \brief Default CTOR

      The default CTOR creates an invalid Addressable object
  */
  Addressable() : Region(NULL), Length(0) {}
  /*! \brief Standard CTOR

      This is the standard CTOR for creating a new Addressable.
  */
  Addressable(const void * _region, size_t _length) : 
    Region(_region), Length(_length) {}
  /*! \brief Copy CTOR

      This copies the Addressable object. However, it does
      not copy anything of the memory addressed by the object.
  */
  Addressable(const Addressable& m) : 
    Region(m.Region), Length(m.Length) {}
  /*! \brief Assignment operator

      This copies the Addressable object. However, it does
      not copy anything of the memory addressed by the object.
  */  
  Addressable& operator=(const Addressable& m) {
    Region = m.Region;
    Length = m.Length;
  }
  /*! \brief Sub-Space pseudo-CTOR

      This function creates an Addressable object, which is
      contained in the current object or invalid. The Addressable
      will start at offset inside the current object and extend to
      the end of the current one.

      \param offset Offset into the object to ignore in the newly created Addressable.
  */
  Addressable& slice(const size_t offset) const {
    return Addressable(ptr(offset),(offset < Length)? Length - offset : 0);
  }
  /*! \brief Sub-Space pseudo-CTOR

      This function creates an Addressable object, which is
      contained in the current object or invalid. The Addressable
      will start at offset inside the current object. If length
      is positive, it wil be used as desired length for the new
      Addressable. If it is negative, it will be used as offset
      from the end.

      \param offset Offset into the object to ignore in the newly created Addressable.
      \param length Length of the new object or, if negative, number of octets to spare at the end of the current Addressable.
  */
  Addressable& slice(const size_t offset, const ssize_t length) const {
    if(length >= 0){
      ssize_t xl = Length - offset;
      return Addressable(ptr(offset),(xl > length)? length : 0);
    } else {
      ssize_t xl = Length - offset;
      // length is negative!
      xl += length;
      return Addressable(ptr(offset),(xl > 0)? xl : 0);
    }
    // cannot be reached!
  }
  /*! \brief Sub-Space pseudo-CTOR

      This function creates an Addressable object, which is
      contained in the current object or invalid. The Addressable
      will start at p inside the current object and extend to the
      end.

      \param p First address of slice
  */
  Addressable& slice(const void *p) const {
    o = offset(p);
    if((o < 0) || (o > Length)) return Adressable();
    return Adressable(p,(o<Length)? Length-o: 0);
  }
  /*! \brief Length of the Addressable in octets 
   */
  size_t len() const { return Length; }
  /*! \brief Start address of the Addressable
   */
  const void *ptr() const { return Region; }
  /*! \brief Offset start address of the Addressable
   */
  const void *ptr(const size_t& offset) const {
    if( offset >= Length ) return NULL;
    const byte *p = static_cast<const byte *>(Region);
    return static_cast<const void *>(p+offset);
  } 
  /*! \brief Last valid address in the Addressable
   
      This function is designed for iterators, which could
      use the following idiom for maximum performance:

      \code
      if(A.limit(sizeof(myType)) != NULL) 
        for(const myType *p = A.ptr(); p <= A.limit(sizeof(myType)); ++p)
      \endcode

      \param l Size of object required to be readable
      \return Last pointer valid for reading an object of l octets inside the Addressable or NULL, if no such object fits

      \note limit() is valid or NULL, so the condition in the for loop
      requires equality. Since ptr() may also be NULL, the condition
      ptr() == limit() == NULL, must be considered. The most efficient
      way is to check whether limit() != NULL. The optimizer should call
      limit() only a single time for the code segment shown above.
  */
  const void *limit(const size_t l = 1) const { 
    return (Length >= l)? ptr(Length - l) : NULL;
  }
  /*! \brief Determine the offset of an address into the Addressable

      This function just calculates the pointer difference with respect
      to the start of the Addressable. It does not check, whether
      the pointer is contained in the Addressable.

      \param p Address to calculate the offset for.
      \return number of octets for p - A.Region

      \sa contains()
   */
  ssize_t offset(const void *p) const {
    const byte *bp = static_cast<const byte *>(p);
    const byte *br = static_cast<const byte *>(Region);
    return static_cast<ssize_t>(bp - br);
  }
  /*! \brief Check whether the Addressable actually contains something
   */
  bool isValid() const { return ((Region != NULL) && (Length > 0)); }
  /*! \brief Check whether an address or region is contained in the Addressable

      This function can be used to check whether a pointer is inside
      the Addressable and whether a length of octets following the
      latter is also contained. When no length is given, the function
      verifies pointer containment, i.e. at least the octet indexed by
      the pointer is contained, if the function returns true.

      \param p Start address to check containment for
      \param length Length of octets to be assured starting from p

      \result True if the indexed addressable space is contained.
  */
  bool contains(const void *p, const size_t length = 0) const {
    if( !p || !isValid()) return false;
    ssize_t o = offset(p);
    if(o < 0) return false;
    // This should work even if ssize_t wrapped
    // The case p == NULL has been caught before
    if (ptr(o) != p) return false;
    // In case length is 0, the optimizer should kick the rest,
    // otherwise, it will catch integer wraps
    if ((offset + length) < (offset | length)) return false;
    return ((offset + length) <= Length);
  }
  /*! \brief Check whether an Addressable object is contained within this object
      Convenience overload function.

      \sa contains(const void *p, const size_t length = 0)
  */
  bool contains(const Addressable& r) const {
    return contains(r.Region, r.Length);
  }
  /*! \brief Safe pointer arithmetic on the Addressable

      This function adds the increment, either positive or
      negative, as an octet length to the pointer passed and
      checks, whether starting from this new pointer a length
      as given by the absolute value of increment is contained
      in the Addressable.

      This funtion is useful for seek() type operations, where
      the Addressable is scanned in either direction in units
      of some record.

      \param p Address to add increment octets to. This value is changed accordingly, or NULL, if there is no next().
      \param increment Number of octets to add to p and to check for remaining length available
      \return True, if the pointer and requested length is valid; false otherwise.

      \sa contains(const void *p, const size_t length = 0)
      \sa add(const void *&p,const ssize_t increment)
  */
  bool next(const void *&p,const ssize_t increment) const {
    const byte *bp = static_cast<const byte *>(p);
    bp += increment;
    if(contains(bp,(increment<0)?-increment:increment)){
      p = static_cast<const void *>(bp);
      return true;
    } else {
      p = NULL;
      return false;
    }
  }
  /*! \brief Safe pointer arithmetic on the Addressable

      This function adds the increment, either positive or
      negative, as an octet length to the pointer passed and
      checks, whether the resulting pointer is contained in 
      the Addressable.

      \param p Address to add increment octets to.
      \param increment Number of octets to add to p
      \return p+increment or NULL, if the result is outside the Addressable
  */
  const void *add(const void *p,const ssize_t increment) const {
    const byte *bp = static_cast<const byte *>(p);
    bp += increment;
    return (contains(bp))? bp : NULL;
  }
  /*! \brief Align a pointer to an access boundary

      This function aligns the pointer passed to the next boundary
      of a bits octet mask, e.g. bits=1: align at 2^1=2, i.e. shorts.
      It returns the aligned pointer or NULL, if such an object
      is not contained in the Addressable starting from the
      aligned pointer.

      \param p pointer to align
      \param bits log2 mask of alignment boundary
      \return Aligned pointer or NULL
  */
  const void *alignBits(const void *p, const size_t bits) const {
    const byte *bp = static_cast<const byte *>(p);
    bp += (1 << bits) - 1;
    bp &= ~((1 << bits) - 1);
    return (contains(bp, 1 << bits))? bp : NULL;
  }
  /*! \brief Align a pointer to an access boundary

      This function aligns the pointer passed to the next boundary
      of a bits octet mask, e.g. bits=1: align at 2^1=2, i.e. shorts.
      It returns the aligned pointer or NULL, if such an object
      is not contained in the Addressable starting from the
      aligned pointer.

      \param p pointer to align
      \param bits log2 mask of alignment boundary
      \return Aligned pointer or NULL

      \note This is an efficiency template, since usually alignment
      boundaries are fixed and known at compile time.

      \sa alignBits()
  */
  template<size_t bits> const void *_alignBits(const void *p) const {
    const byte *bp = static_cast<const byte *>(p);
    bp += (1 << bits) - 1;
    bp &= ~((1 << bits) - 1);
    return (contains(bp, 1 << bits))? bp : NULL;
  }
  /*! \brief Align pointer to word (2 octets) boundary

      \param p pointer to align
      \return Aligned pointer or NULL

      \sa alignBits(), _alignBits()
  */  
  const void *alignWord(const void *p) const {
    return _alignBits<1>(p);
  }
  /*! \brief Align pointer to double word (4 octets) boundary

      \param p pointer to align
      \return Aligned pointer or NULL

      \sa alignBits(), _alignBits()
  */  
  const void *alignDWord(const void *p) const {
    return _alignBits<2>(p);
  }
  /*! \brief Align pointer to quad word (8 octets = 64 bit) boundary

      \param p pointer to align
      \return Aligned pointer or NULL

      \sa alignBits(), _alignBits()
  */  
  const void *alignQWord(const void *p) const {
    return _alignBits<3>(p);
  }
  /*! \brief Align a pointer to an access boundary

      This function aligns the pointer passed to the next boundary
      as determined by log2 of the size given. This is a
      convenience overload for writing code, where the size
      of a variable is unknown at compile time.
     
      It returns the aligned pointer or NULL, if such an object
      is not contained in the Addressable starting from the
      aligned pointer.

      \param p pointer to align
      \param s size of the object to align
      \return Aligned pointer or NULL

      \sa alignBits(), alignSizeOf()
  */  
  const void *alignSize(const void *p, size_t s) const {
    size_t bits = 0;
    for(size_t ts; ts; ++bits, ts >> 1);
    if(bits){
      size_t ts = 1 << (bits - 1);
      if( s > ts ) ++bits;
      return alignBits(p,bits);
    }
    return (contains(p)? p : NULL);
  }  
  /*! \brief Align a pointer to an access boundary

      This function aligns the pointer passed to the next boundary
      as determined by log2 of the size given. This is an
      efficiency template for writing code, where the size
      of a variable is unknown during programming, but known at 
      compile time.
     
      It returns the aligned pointer or NULL, if such an object
      is not contained in the Addressable starting from the
      aligned pointer.

      \code
      double value = SOME_VALUE;
      if(NULL != (p = A.alignSizeOf< sizeof(double) >(p))) *p = value;
      \endcode

      \param p pointer to align
      \param s size of the object to align
      \return Aligned pointer or NULL

      \sa alignBits(), alignSize()
  */  
  template<size_t s> const void *alignSizeOf(const void *p) const {
    return _alignBits< meta::UpperLog2< s >::RESULT >(p);
  }
};

  /*! \class PoolEntry 
      \brief A class representing chunks of used or unused memory in a continuous memory pool
  */
  class PoolEntry {
  public:
    typedef unsigned char octet;  //!< data type occupying one address count

    union {
      struct {
	size_t size;              //!< size of the block including the entry
	void *align_payload;      //!< for reference only; payload starts here
      } used;
      struct {
	size_t size;              //!< size of the block including the entry
	PoolEntry *last;          //!< previous empty block
	PoolEntry *next;          //!< next empty block
      } unused;
    } entry;

    enum {
      USED_BIT = 0                //!< this bit of the size is the used flag
    };

    enum {
      USED_MASK = 1 << USED_BIT   //!< mask value for USED_BIT
    };

  protected:
    /*! \brief Set used / unused flag

        Note that it only sets the flag. It does
	not handle the unused list.
    */
    void setUsed( bool used = true ){
      if(used){
	entry.unused.size |= PoolEntry::USED_MASK;
      } else {
	entry.used.size &= ~PoolEntry::USED_MASK;
      }
    }

    /*! \brief Initialize a pool entry

        This CTOR is usually called in the context of
	a placement new, since the memory is allocated
	in the pool.

	Note that for unused initialization, it does
	not add the entry to the unused list.
    */
    PoolEntry(size_t s, bool used = false){
      if(used){
	entry.used.size = s;
	setUsed(true);
      } else {
	entry.unused.size = s;
	setUsed(false);
      }
    }

  public:
    /*! \brief Check whether this PoolEntry is used or not
     */
    bool isUsed() const {
      return (0 != (entry.used.size & PoolEntry::USED_MASK));
    }
    /*! \brief Return size of this block

        This function strips all flags also merged into
	the size parameter.
    */
    size_t size() const {
      return entry.used.size &= ~PoolEntry::USED_MASK;
    }
    /*! \brief Test for the final unused block holding the list head
     */
    bool isFinal() const {
      if(isUsed()) return false;
      if(entry.unused.next < this) return true;
      return false;
    }
    /*! \brief Get next block in contiguous pool

        The end of a pool is marked by a mandatory unused
	block, which holds the head of the unused list. This
	block will return a valid next(), which is outside the
	pool. The programmer shall assure that next() is not
	applied to this final block.

	\sa isFinal()
    */	
    PoolEntry *next() const {
      return static_cast<PoolEntry *>(static_cast<byte *>(this)+size());
    }

    /*! \brief Unused list iterator
     */
    PoolEntry *nextUnused() const {
      if(isUsed()){
	PoolEntry *c = this;
	while(c.isUsed()) c = c.next();
	return c;
      }
      return entry.unused.next;
    }

    /*! \brief Unused list iterator
     */
    PoolEntry *previousUnused() const {
      if(isUsed()){
	PoolEntry *c = this;
	while(c.isUsed()) c = c.next();
	return c.entry.unused.last;
      }
      return entry.unused.last;
    }

    /*! \brief Unused list iterator

        A walk through all unused blocks is intended
	using the following construct:

      \code
      PoolEntry *me;  // some PoolEntry from the contiguous pool
      for(PoolEntry *e = me->firstUnused();
          !e->isFinal();
	  e = e->nextUnused()){
         // do something to e
      }         
      \endcode

     */
    PoolEntry *firstUnused() const {
      PoolEntry *c = this;
      while(!c.isFinal()) c = c.nextUnused();
      return c.entry.unused.next;
    }


    /*! \brief Convert a used block to unused

        This function marks a block unused, merges it
	with adjacent unused blocks and hooks the final result
	into the free block list.
    */
    const PoolEntry *free(){
      // find next free block, we expect a final free block at the end
      PoolEntry *n = next();
      while(n->isUsed()) n = n->next();
      // get our predecessor free block
      PoolEntry *p = n->entry.unused.last;
      // we may vanish!
      PoolEntry *c = this;
      
      // check for left merger
      if(p->next() == c){
	p.unused.size = p.size()+c.size();
	c = p;
	// empty lists are okay in this case
      }
      // check for right merger, don't merge with final free block
      if((c->next() == n) &&
	 (n->entry.unused.next > n)){
	c.unused.size = c.size()+n.size();
	c.unused.entry.next = n.unused.next;
	// predecessor is unaffected
      }

      c.setUsed(false);      
      return c;
    }

    /*! \brief Create a used block

        Splits a used block from the PoolEntry beginning at
	its start address. The remainder, if any, may be 
	created as a new unused block, which is added to
	the unused block list.

	The function returns the actual size of the used
	fragment, including the size parameter. If it returns
	0, then the requested size did not fit into the
	PoolEntry.
    */
    size_t allocate(size_t asize){
      // so this one is unused, make it used and eventually fragment
      PoolEntry *p = entry.unused.last;
      PoolEntry *n = entry.unused.next;

      Addressable me(this,size());
      void *end = me.ptr(asize);
      if(!end) return 0;

      end = me.alignSizeOf<sizeof(PoolEntry *)>(end);
      if(!end) return 0;

      if (me.next(end,sizeof(entry))){
	// fragment block
	entry.unused.size = me.offset(end);
	Adressable frag(me.slice( entry.unused.size )); 
	// fix empty list
	PoolEntry *f = new (frag.ptr()) PoolEntry(frag.len(),false);
	f.entry.unused.last = p;
	p.entry.unused.next = f;
	f.entry.unused.next = n;
	n.entry.unused.last = f;
      }
      
      setUsed(true);

      return size();
    }
      
    /*! \brief Get the payload region

        Returns the payload region as an Adressable object,
	if the block is used, or an invalid Adressable,
	otherwise.
    */
    Addressable payload() const {
      if(isUsed()){
	return Addressable(static_cast<void *>(&entry.used.payload),
			   entry.used.size - sizeof(entry.used.size));
      } else {
	return Addressable();
      }
    }

    /*! \brief Initialize the unused block list

        This method effectively creates a contiguous pool
	into the Adressable range passed. The pool is initialized
	to hold the final unused block with the unused list anchor
	and another unused block occupying all the rest, safe for
	alignment issues.
	
	\return Pointer of the final block
    */
    static PoolEntry *create(const Adressable &a){
      void *start = a.alignSizeOf<sizeof(PoolEntry *)>(a.ptr());
      Adressable b(a.slice(-2*sizeof(entry)));
      if(!b.isValid()) return NULL;
      void *end = b.ptr(sizeof(entry));
      end = b.alignSizeOf<sizeof(entry)>(end);
      if(!a.contains(end)) return NULL;
      b = a.slice(end);
      a = a.slice(start);
      ssize_t o = a.offset(end);
      if(o <= sizeof(entry)) return NULL;
      a = a.slice(o);

      if(!a.isValid() || !b.isValid()) return NULL;

      PoolEntry *f = new (a.ptr()) PoolEntry(a.len(),false);
      PoolEntry *l = new (b.ptr()) PoolEntry(b.len(),false);
      f->entry.unused.next = l;
      f->entry.unused.last = l;
      l->entry.unused.next = f;
      l->entry.unused.last = f;

      return l;      
    }

  };

  class LockedPool {
  private:
    const size_t LogPageSize;
    
  public:
    LockedPool(const size_t logPageSize) : LogPageSize(logPageSize) {}
    
  };

/*
template<const int _type = 0, const bool _debug = false>
class Addressable : public MemoryRegion {
public:
  typedef template<> Addressable<_type,_debug> self_t;
  const int type() const { return _type; }
  bool checkAccess(const size_t offset, const size_t length = 1) const {
    if( Region == NULL ) return false;
    if( offset + length > Length) return false;
    if( offset + length < offset | length) return false;
    return true;
  }
  void assertAccess(const size_t&, const size_t&) const {}  
  unsigned char octet(const size_t offset) const {
    assertAccess(offset,1);
    return (static_cast<unsigned char *>(Region))[offset];
  }
  self_t slice(const size_t& offset, const size_t& length) const {
    assertAccess(offset,length);
    unsigned char *p = static_cast<unsigned char *>(Region);
    p += offset;
    return self_t(p,length);
  }
  void copy(const template<const bool _d> Addressable<_type,_d>& a){
    assertAccess(0,a.Length);
    memcpy(Region,a.Region,a.Length);
  }
  void clone(const template<const bool _d> Addressable<_type,_d>& a){
    copy(a);
    shrink(a.Length);
  }
  void blit(const template<const bool _d> Addressable<_type,_d>& a,
	    const size_t length = a.Length,
	    const size_t offset_self = 0,
	    const size_t offset_source = 0){
    template<> Addressable<_type,_d> s = 
      a.slice(offset_source,length);
    self_t d = slice(offset_self,length);
    d.copy(s);
  }          
};

template<const int _type, true>
void assertAccess(const size_t& offset, const size_t& length) const {
  if(!checkAccess(offset,length)) throw std::overflow_error;
}
*/
}; // namespace mgr

#endif // _UTIL_MEMORY_H_
    
