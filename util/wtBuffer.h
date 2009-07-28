/*
 *
 * Extensible reference-counted buffer
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: wtBuffer.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  wtBuffer  - an extensible auto-allocation buffer
 *              with write-through capability
 *
 * This defines the values:
 *  DEFAULT_WTBUFFER_CHUNK
 *
 */

/*! \file wtBuffer.h
    \brief Extensible reference-counted buffer

    This file defines the wtBuffer<> template, which inherits
    from _wtBuffer. _wtBuffer uses __wtBuffer as reference counted
    object.

    \warning The code has a lot of inlined code using defined in the header
    using the ctdbg() macro

    \author Dr. Lars Hanke
    \date 2006    
*/

#ifndef _UTIL_WTBUFFER_H_
# define _UTIL_WTBUFFER_H_

# ifndef DEFAULT_WTBUFFER_CHUNK
/*! \def DEFAULT_WTBUFFER_CHUNK
    \brief Default allocation chunk size

    This macro defines the default allocation chunk size as 128 records.
    If you define this macro before including wtBuffer.h, this will
    override this default, i.e. the _wtBuffer CTOR will behave differently,
    when the chunk size is not specified.
*/
#  define DEFAULT_WTBUFFER_CHUNK 128
# endif

#include <unistd.h>
#include <mgrError.h>
#include <string.h>
#include <stdlib.h>

namespace mgr {

namespace clib {
  using ::free;
  using ::malloc;
  using ::realloc;
  using ::memcpy;
  using ::memset;
};

#ifdef DEBUG_ALLOCATE
#define ctdbg(...) pdbg( __VA_ARGS__ )
#else
/*! \def ctdbg
    \brief Macro for debugging allocation of _wtBuffer in host applications

    If DEBUG_ALLOCATE is defined ctdbg() is mapped to pdbg() and will
    produce detailed messages about when, which and how much memory
    is claimed or released. This may be quite useful for chasing memory
    leaks.
*/
#define ctdbg(...) while(0)
#endif

/*! \class _wtBuffer
    \brief Reference counted array - core class

    wtBuffer is designed to work with large amounts of data, where
    unnecessary copies are painful for both memory usage and time.
    As a very special feature wtBuffer can reference its initialisation
    data without even copying a single instance.

    Another design goal is to obtain plain C data pointers for
    compatibility with C-style libraries and hardware. wtBuffer is the
    buffer of choice for working with data-acquisition and data-processing
    especially, if e.g. numerical recipes or similar libraries shall be
    used.
 
    This class is intended as a wrapper for C-style arrays as objects
    using reference counting. The real interface class is the wtBuffer
    template class. _wtBuffer implements the more abstract concept of
    a reference counted memory region, which has more space allocated
    than used and a well defined record size.

    Additional features are memory management by allocation in chunks
    and the option to immediately reference constant data, i.e. copying
    of initialisation data strictly follows the late or lazy allocation
    paradigma.

    This class embeds __wtBuffer as reference counted container.

    _wtBuffer in contrast to wtBuffer is void typed and sizes are
    assumed as octets in general.

    \todo branch() should produce a writeable pointer in order to
    have a canocial way to obtain such a pointer.
*/
class _wtBuffer {
  /*! BerContentRegion does some hacks to allow overcoming record sizes
      This should vanish, when record sizes are discarded from _wtBuffer
  */
  friend class BerContentRegion;

protected:
/*! \class __wtBuffer
    \brief The actual reference counted container used with _wtBuffer.

    __wtBuffer maintains a region of space with a reference count.    
*/
class __wtBuffer {
private:
  size_t rCount;        //!< The reference counter

protected:
  size_t allocated;     //!< Size in octets of the allocated buffer
  void *buffer;         /*!< \brief Start address of the buffer, 
			     or NULL if no buffer allocated */

public:
  /*! \brief Buffer resizing
      \param s new size of buffer
      \return new buffer address, or NULL if allocation failed

      Resizes the buffer preserving its contents. The function uses the
      C-library memory allocation functions malloc(), realloc(), and free()
      If resizing fails, the buffer contents are lost.
  */
  void *resize(const size_t& s){
    if(s == allocated) return buffer;
    allocated = s;
    if(allocated){
      if(!buffer){
	buffer = mgr::clib::malloc(s);      
      } else {
	buffer = mgr::clib::realloc(buffer,s);
      }
      if(!buffer) allocated = 0;
    } else {
      if(buffer) mgr::clib::free(buffer);
      buffer = NULL;
    }
    return buffer;
  }

  /*! \brief CTOR for all purposes other than copy CTOR
      \param s size of buffer to allocate
      \param d data to initialize buffer from

      If the requested buffer size is 0, no buffer is allocated
      and of course it is not initialised. If no initialisation
      data is supplied the buffer may contain arbitrary data, i.e.
      it is not zeroized or anything like that.
     
      Due to its default parameters its use as unspecific CTOR, i.e.
      __wtBuffer myBuffer; is sensible.
  */
  __wtBuffer(size_t s = 0, const void *d = NULL) : rCount(0), allocated(s) {
    if(allocated) buffer = mgr::clib::malloc(allocated);
    else buffer = NULL;
    if(!buffer) allocated = 0;
    else if(d){
      mgr::clib::memcpy(buffer,d,allocated);
    }
    ctdbg("### Created __wtBuffer %p (%p)\n",this,buffer);
  }  

  /*! \brief The deep copy CTOR

      This CTOR does a deep copy of its argument. It does not
      produce a new reference.

      \sa void *resize(const size_t& s) 
      \sa __wtBuffer& operator=(const __wtBuffer& b)
  */
  __wtBuffer(const __wtBuffer& b) : rCount(0), buffer(NULL) {
    resize(b.allocated);
    ctdbg("### Copy-Created __wtBuffer %p (%p)\n",this, buffer);
  }

  /*! \brief Destructor frees Buffer
  */
  ~__wtBuffer(){
    ctdbg("### Destroy __wtBuffer %p (%p)\n",this,buffer);
    if(buffer) mgr::clib::free(buffer);
  }
  
  /*! \brief Deep copy assignment

      The assignment operator does a deep copy of its argument. It does not
      produce a new reference.

      \sa __wtBuffer(const __wtBuffer& b)
      \todo resize() uses realloc(), which might cause one overhead copy
  */      
  __wtBuffer& operator=(const __wtBuffer& b){
    if(resize(b.allocated)) mgr::clib::memcpy(buffer,b.buffer,allocated);
    return *this;
  }      

  /*! \brief Deep copy of buffer contents, only

      The content assignment operator does a deep copy of the contents
      of its argument. The length of the current buffer is not changed.
      If the buffer passed exceeds the length of the buffer, it is 
      truncated to fit the buffer. If it is shorter, the original buffer
      contents remain after the newly copied contents.
  */
  __wtBuffer& operator<<(const __wtBuffer& b){
    size_t s = allocated;
    if(s > b.allocated) s = b.allocated;
    mgr::clib::memcpy(buffer,b.buffer,s);
    return *this;
  }

  /*! \brief Retrieve buffer pointer as (void *)
      \return buffer

      This method implements read-only access to the buffer property.

      \sa buffer cptr()
  */
  void *ptr() const { return buffer; }

  /*! \brief Retrieve buffer pointer as (char *)
      \return buffer as (char *)

      This method implements read-only access to the buffer property.
      In contrast to ptr() it returns a (char *) in order to allow
      for pointer arithmetics, since there is no such thing as
      void *p + 1.

      \sa buffer ptr()
  */
  char *cptr() const { return static_cast<char *>(buffer); }

  /*! \brief Retrieve allocated buffer size

      This method implements read-only access to the allocated property.

      \sa allocated
  */
  const size_t& size() const { return allocated; }

  /*! \brief Replace __wtBuffer contents with new data

      \param d Start address of data
      \param s Length of data in octets
      \return ERR_NO_ERROR or ERR_MEM_AVAIL, if reallocation failed

      Reads the data specified into the current __wtBuffer. If the size
      s exceeds the allocated size, the buffer is enlarged using resize().
      Trailing octets are set to 0x00 if exist.
  */  
  m_error_t read(const void *d, size_t s){
    if(s > allocated) 
      if(!resize(s)) return ERR_MEM_AVAIL;
    mgr::clib::memcpy(buffer,d,s);
    if(s < allocated){
      char *t = cptr();
      t += s;
      mgr::clib::memset(t,0,allocated - s);
    }
    return ERR_NO_ERROR;
  }

  /*! \brief Increment reference counter

      lock() shall be called before the buffer is accessed the first 
      time, otherwise it may change or disappear.

      \sa release()
  */
  void lock(void){ ++rCount; };

  /*! \brief decrement reference counter
      \return bool, if release() returns true, the __wtBuffer instance
      can be deleted
  
      release() shall be called after the buffer was accessed the last
      time. You should call release() for every lock(). __wtBuffer does
      not perform any bookkeeping on who locks or releases.
  */
  bool release(void){ 
    // avoid wrapping
    if(!rCount) return true;
    return (--rCount == 0);
  }

  /*! \brief Verify whether buffer modifications may have side effects

      \return false if the buffer has only a single client and may be
      modified freely

      If multiple instances reference the current __wtBuffer, then these
      may have their working copies of ptr(). You must not resize(), if the 
      buffer isShared(). resize() will not enforce this, since you might have
      good reasons. Unless you know exactly what you are doing, create your
      own copy of the buffer using __wtBuffer( *this ), and do yout 
      modifications there.

      \sa lock() release() __wtBuffer( const __wtBuffer& )
  */
  bool isShared(void) { return (rCount > 1); }


  /*! \brief Retrieve allocated buffer size

      \deprecated use size() instead, may be removed in future versions

      This method implements read-only access to the allocated property.

      \sa allocated
  */
  const size_t &AllocSize() const {
    return allocated;
  }
};          

  size_t chunk;      //!< size of allocation chunk in bytes
  size_t length;     //!< buffer length in bytes
  bool accessWrite;  /*!< \brief automatically copy, if non constant 
		          access is claimed for constant buffer */
  bool isFix;        //!< how to interpret the buf union
  
  union {
    __wtBuffer *var; //!< pointer to reference counted container
    const void *fix; //!< pointer to initialisation data
  } buf;             /*!< \brief storage for pointer to be interpreted 
		          according to isFix */

  //! set-up values for empty buffer
  void initEmpty(){
    isFix = true;
    length = 0;
    buf.fix = NULL;
  }

  //! set-up writeable buffer
  m_error_t initVar(const size_t&s);

  /*! \brief CTOR core function
      \param c size of allocation chunk in bytes

      If c is given re-allocation is performed in chunks of c,
      otherwise c will default to DEFAULT_WTBUFFER_CHUNK, which can be
      overridden for project or even file defaults.
  */      
  void init_wtBuffer(const size_t& c=DEFAULT_WTBUFFER_CHUNK);

  //! create a writeable instance
  m_error_t branch(const size_t& s);

  //! bytes to allocate for requested number of bytes
  /*! \param l requested bytes to allocate
      \retval error error code as defined in mgrError.h
      \return number of bytes rounded up to full chunks

      When a __wtBuffer is to be enlarged chunk contains the size
      of new portions of memory. This function computes the byte
      size of the smallest n chunk region containing l bytes.

      \note error is left untouched, if no error occurs. The caller
      is responsible to reset error before calling.

      \sa roundRecChunk
  */
  inline size_t roundChunk(const size_t& l, m_error_t& error){
    if(!chunk){
      error = ERR_INT_STATE;
      return l;
    }

    size_t rl = l % chunk;
    rl = chunk - rl;

    return l + rl;
  }

  //! bytes to allocate for requested number of records
  /*! \param l requested records to allocate
      \return number of bytes rounded up to full chunks

      When a __wtBuffer is to be enlarged chunk contains the size
      of new portions of memory. This function computes the byte
      size of the smallest n chunk region containing l records.

      \sa roundChunk
  */
  /*
    pass this to wtBuffer<>

  inline size_t roundRecChunk(const size_t& l){
    if(!record || !chunk){
      error = ERR_INT_STATE;
      return l;
    }
    size_t rl = chunk - (l % chunk);
    rl += l;
    rl *= record;

    return rl;
  }
  */
                
 public:
  /*! \brief Constructor without initialization
      \param c chunk size

      Creates an empty _wtBuffer, which is well typed concerning
      its memory handling and data size.

      \sa init_wtBuffer()
  */
  _wtBuffer(const size_t& c =  DEFAULT_WTBUFFER_CHUNK){
    init_wtBuffer(c);
    length = 0;
    buf.fix = NULL;    
  }

  /*! \brief Constructor with initialization
      \param data address of initialisation data
      \param l size of data in octets
      \param c chunk size

      Creates a _wtBuffer, which is well typed concerning
      its memory handling and data size. The initialization 
      data is not copied into its own buffers, but only referenced.
      Copying will be performed, if write access is requested.

      \sa init_wtBuffer() branch()
  */
  _wtBuffer(const void *data, const size_t &l, const size_t& c =  DEFAULT_WTBUFFER_CHUNK){
    init_wtBuffer(c);    
    length = l;
    buf.fix = data;
  }

  /*! \brief Discard buffer contents
    
      Releases the lock on __wtBuffer if exists and also deletes
      the __wtBuffer instance, if __wtBuffer::release() allows.
      The buffer is initialized as empty.

      \sa initEmpty()
  */
  void free(void){
    if(!isFix){
      ctdbg("### _wtBuffer:free() %p->%p\n",this,buf.var);
      if(buf.var->release()) delete buf.var;
      buf.var = NULL;
    }
    initEmpty();
  }

  //! Copy constructor
  _wtBuffer(const class _wtBuffer& b);

  /*! \brief Destructor
    
      Essentially calls free() before discarding itself.
  */
  virtual ~_wtBuffer(void){
    ctdbg("### DTOR _wtBuffer @ %p\n",this);
    free();
  }

  //! create a writeable instance
  /*! Rounds the current length to chunk size and creates
      a new instance of __wtBuffer, unless the buffer is
      writeable and used exclusively by the current instance.
      Otherwise, the content is copied.

      \sa roundChunk()
  */
  m_error_t branch(){
    m_error_t error = ERR_NO_ERROR;
    size_t s = roundChunk(length, error);
    if(error != ERR_NO_ERROR) return error;
    return branch(s);
  }

  //! Assignment operator
  class _wtBuffer& operator=(const class _wtBuffer& b);

  //! Change size preserving contents
  /*! \param l new size in bytes
      \param copy allow enlarging fixed buffers through copy      
      \return error code as defined in mgrError.h

      trunc() heeds accessWrite as default for allowing to
      copy referrals.
  */
  m_error_t trunc(const size_t& l, bool copy);

  /*! \brief Change size preserving contents
      \param l new size in bytes
      \return error code as defined in mgrError.h

      This function defaults the copy parameter
      of trunc(const size_t& l, bool copy)
      with the current setting of accessWrite.
  */
  m_error_t trunc(const size_t& l){
    return trunc(l, accessWrite);
  }

  //! Discard buffer and allocate new space
  m_error_t allocate(const size_t& l);

  //! change length explicitly without re-allocation
  size_t accept(const size_t& s);

  /*! \brief return pointer to data
      \return pointer to data

      Returns the fixed data pointer for references or the
      storage area pointer from a __wtBuffer container.

      This pointer is always a const void *. If you request write access,
      you'll have to check, whether the memory may be written or not and 
      subsequently do a const_cast.

      \sa isWriteable()
  */
  inline const void *rawPtr(void) const{
    if(isFix) return buf.fix;
    if(!buf.var) return NULL;
    return buf.var->ptr();
  }

  //! Inequality operator
  bool operator!=(const _wtBuffer& b) const;

  //! Equality operator
  /*! This is the logic negation of operator!=(). Since two NULL
      pointer are considered different, they are not equal.

      \sa operator!=()
  */
  bool operator==(const _wtBuffer& b) const {
    return !operator!=(b);
  }
  
  //! Check whether a buffer can produce a writeable instance
  /*! Referrals are not writeable, unless copying of the original
      source data to a __wtBuffer instance is allowed. The function
      returns false for such read-only buffers. Returning true means
      that a writeable instance can be produced. This does not imply
      that the current rawPtr() may be written. If in doubt, always 
      call branch before const casting the rawPtr().
  */
  inline bool isWriteable(void) const {
    return (!isFix || accessWrite || (isFix && (buf.fix == NULL)));
  }

  //! set the accessWrite property
  /*! \param wr bool value to set accessWrite to
   */
  inline void setWriteable(bool wr = true){
    accessWrite = wr;
  }

  //! Return the chunk size in bytes
  inline const size_t& Chunk(void) const {
    return chunk;
  }

  //! Set the chunk size in bytes
  /*! \param c new chunk size in bytes
      \return ERR_PARAM_RANG if chunk size is not acceptable, i.e. zero

      Setting a new chunk size does not immediately affect the buffer,
      but will change the allocation behaviour the next time allocation
      is performed.
  */
  inline m_error_t Chunk(const size_t& c){
    if(!chunk) return ERR_PARAM_RANG;
    chunk = c;
    return ERR_NO_ERROR;
  }

  //! get valid size of the buffer in octets
  inline const size_t & byte_size(void) const 
    { return length; };

  /*! \brief Return the size of the allocated area

      If the buffer is a referral, i.e. has no allocated
      memory, the current length is returned. A non-zero
      alloc_size() does not indicate that the buffer is
      writeable.
  */
  inline const size_t alloc_size(void) const {
    if(isFix) return byte_size();
    if(!buf.var) return 0;
    return buf.var->AllocSize();
  }

  //! Version information string
  const char * VersionTag(void) const;

  // now the more interesting stuff

  /*! \brief replace data in buffer discarding previous data
      \param data start address of initialisation data
      \param l size of data in octets
      \return error code as defined in mgrError.h

      The function discards all previous contents and eventually 
      releases a __wtBuffer. The new initialisation content is
      taken as referral, i.e. you must branch() before write.
  */
  inline m_error_t replace(const void *data, const size_t& l){    
    if(!isFix) free();
    buf.fix = data;
    length = l;
    return ERR_NO_ERROR;
  }

  /*! \brief append data to current buffer
      \param data start address of data to append
      \param l length of data to append
      \return error code as defined in mgrError.h

      The function appends the data to the contents currently
      in the buffer. The new data is copied and the buffer will
      refer to a __wtBuffer instance afterwards. If it already
      is a __wtBuffer reallocation only occurs, if there is not
      enough space left in the allocated area.
      
      The contents appended will immediately follow the
      contents originally present in the buffer.
  */
  inline m_error_t append(const void *data, const size_t& l){
    size_t old = length;
    m_error_t error = trunc(length + l, true);
    if(error != ERR_NO_ERROR) return error;
    memcpy(buf.var->cptr() + old, data, l);
    return ERR_NO_ERROR;
  }

  /*! \brief append another buffer
      \param b reference to a _wtBuffer
      \return error code as defined in mgrError.h
      
      This is overloaded to append(const void *data, const size_t& l).
      Data start and length are retrieved from the buffer
      referenced.
  */      
  inline m_error_t append(const class _wtBuffer& b){
    return append(b.rawPtr(), b.length);
  }

  /*! \brief prepend data to current buffer
      \param data start address of data to prepend
      \param l length of data to prepend
      \return error code as defined in mgrError.h

      The function prepends the data to the contents currently
      in the buffer. The new data is copied and the buffer will
      refer to a __wtBuffer instance afterwards. If it already
      is a __wtBuffer reallocation only occurs, if there is not
      enough space left in the allocated area, i.e. if possible the 
      original contents will be moved inside the allocated area before
      the prepended data is copied.
      
      The contents prepended will be immediately followed by the
      contents originally present in the buffer.
  */
  m_error_t prepend(const void *data, const size_t& l);

  /*! \brief prepend another buffer
      \param b reference to a _wtBuffer
      \return error code as defined in mgrError.h
      
      This is overloaded to prepend(const void *data, const size_t& l).
      Data start and length are retrieved from the buffer
      referenced.
  */      
  inline m_error_t prepend(const class _wtBuffer& b){
    return prepend(b.rawPtr(), b.length);
  }

  /*! \brief Insert / Overwrite into buffer
      \param at position for insert / overwrite, 
      i.e. length of original data to leave untouched.
      \param consume amount of data to remove following at
      \param data start address of data to insert
      \param l length of data to insert
      \return error code as defined in mgrError.h

      This function works similar to 'mark and paste' in most editors.
      The contents from position at lengthed by consume are replaced by
      the l bytes at data. The function makes no assumptions concerning
      l and consume, except that at + consume < length.

      This is the basic interface to all overloaded methods.
  */
  m_error_t insert(const size_t& at, const size_t& consume, const void *data, const size_t& l);

  /*! \brief Insert into buffer
      \param at position for insert / overwrite, 
      i.e. length of original data to leave untouched.
      \param data start address of data to insert
      \param l length of data to insert
      \return error code as defined in mgrError.h

      This function inserts the l bytes from data in between
      at and at+1 into the buffer. None of the original
      contents will be overwritten.
  */
  m_error_t insert(const size_t& at,const void *data, const size_t& l){
    return insert(at, 0, data, l);
  }

  /*! \brief Insert / Overwrite into buffer
      \param at position for insert / overwrite, 
      i.e. length of original data to leave untouched.
      \param consume amount of data to remove following at
      \param b buffer holding contents to insert
      \return error code as defined in mgrError.h

      This is an overloaded method for
      insert(const size_t& at, const size_t& consume, const void *data, const size_t& l)
      where data and l are extracted from b.
  */
  m_error_t insert(const size_t& at, const size_t& consume, const class _wtBuffer& b){
    return insert(at, consume, b.rawPtr(), b.length);
  }

  /*! \brief Insert / Overwrite into buffer
      \param at position for insert / overwrite, 
      i.e. length of original data to leave untouched.
      \param b buffer holding contents to insert
      \return error code as defined in mgrError.h

      This is an overloaded method for
      insert(const size_t& at, const void *data, const size_t& l)
      where data and l are extracted from b.
  */
  m_error_t insert(const size_t& at, const class _wtBuffer& b){
    return insert(at, 0, b);
  }

  /*! \brief get record size
      \return record size

      A feature of wtBuffer is that data using a record structure can be
      handled using generic functions, e.g. for I/O. Data shell be assumed
      as an array of data items of constant length, i.e. records, if this
      function returns a positive length. Negative or zero-length may
      be returned by other data. If data is void as with _wtBuffer the length
      shall be 0.

      This is a virtual interface to overload with the final implementation,
      when record sizes are defined.
  */
  virtual const ssize_t rec_size(void) const {
    return 0;
  }
};

template <class T> class wtBuffer : public _wtBuffer {
protected:  
  /*! \brief Obtain read only pointer to buffer of correct type
      \return Constant pointer to buffer
        
      This is just a typing wrapper for _wtBuffer::rawPtr(). Returns
      NULL if the buffer does not hold data.
  */
  inline const T *get_fix(void) const { 
    return static_cast<const T *>(rawPtr()); 
  }
  /*! \brief Obtain a writeable pointer of correct type
      \param error variable to hold error code returned by branch()
      \return Non-constant pointer to buffer

      This performs a _wtBuffer::branch() to ensure the pointer
      is a writeable buffer and then retrieves the pointer
      immediately from __wtBuffer. It returns NULL, if the buffer
      could not be branched.
  */
  inline T *get_var( m_error_t& error ) { 
    error = branch();
    if( ERR_NO_ERROR != error ) return NULL;
    return static_cast<T *>((!isFix)? (buf.var->ptr()) : NULL); 
  }

  /*! \brief checks, if a byte size fulfills record boudaries
      \param l size in bytes
      \return true if l is a multiple of the record size
  */
  inline bool check_recs( const size_t& l ){
    return ( 0 == (l % sizeof(T)));
  }

  /*! \brief align byte size to record boundary
      \param size in bytes
      \return size in bytes aligned to record boundary

      This function rounds down to record size, i.e. fragments
      of records trailing the real content are discarded.
  */
  inline const size_t& roundRecs( const size_t& l ){
    size_t rl = l % sizeof(T);
    return l - rl;
  }

public:
  /*! \brief Standard Constructor
      \param c Chunk size in records, defaults to DEFAULT_WTBUFFER_CHUNK

      This is equivalent to the CTOR of _wtBuffer, but
      the chunk size is now in records instead of bytes.
  */
  wtBuffer(const size_t& c = DEFAULT_WTBUFFER_CHUNK) : 
    _wtBuffer(c * sizeof(T)) {}

  /*! \brief Initializing Constructor
      \param data array of data type T for initialisation
      \param l number of data items in the array
      \param c Chunk size in records, defaults to DEFAULT_WTBUFFER_CHUNK

      This is equivalent to the CTOR of _wtBuffer, but
      the chunk size and data length is now in records instead of bytes.
  */
  wtBuffer(const T *data, const size_t &l, 
	   const size_t& c = DEFAULT_WTBUFFER_CHUNK) : 
    _wtBuffer(data, l*sizeof(T), c * sizeof(T)) {}

  /*! \brief Destructor
    
      The default destructor. It is virtual since the class implements
      virtual functions.
  */
  virtual ~wtBuffer() {}

  /*! \brief Copy constructor
      \param b Buffer of same type to copy

      Behaviour of this copy constructor is identical to
      the _wtBuffer copy constructor.
  */
  wtBuffer(const class wtBuffer<T>& b) : _wtBuffer(b) {}

  /*! \brief Assignment operator
      \param b Buffer of same type to copy

      Behaviour of this assignment operator is identical to
      the _wtBuffer assignment operator.
  */
  inline class wtBuffer<T>& operator=(const class wtBuffer<T>& b){
    return static_cast<wtBuffer<T>& >(_wtBuffer::operator=(b));
  }

  /*! \brief Report the record size in bytes for use with 
      abstract functions defined with _wtBuffer objects.
  */
  virtual const ssize_t rec_size(void) const {
    return sizeof(T);
  }

  /*! \copydoc _wtBuffer::trunc(const size_t&, bool)

      The wtBuffer version uses records for size
      instead of bytes.
  */
  m_error_t trunc(const size_t& l, bool copy){
    return _wtBuffer::trunc( l * sizeof(T), copy);
  }  

  /*! \copydoc _wtBuffer::trunc(const size_t&)

      The wtBuffer version uses records for size
      instead of bytes.
  */
  m_error_t trunc(const size_t& l){
    return _wtBuffer::trunc( l * sizeof(T));
  }

  /*! \fn allocate(const size_t& l)
      \copydoc _wtBuffer::allocate(const size_t& l)
      \deprecated This is a legacy function operating
      on bytes. Use allocateRecs() instead.
  */
  /*! \brief Allocate space discarding previous contents
      \param l size of memory to allocate in records
      \return error code as defined in mgrError.h

      This is a wrapper function for
      _wtBuffer::allocate(const size_t& l). Refer to the
      latter for more details.
  */
  m_error_t allocateRecs(const size_t& l){
    return  _wtBuffer::allocate(l * sizeof(T));
  }

  /*! \brief Change valid length of buffer
      \param s New length of buffer in records
      \return Actual new length of buffer

      This function changes the length of valid
      data inside the buffer. It does not re-allcoate, but
      it could be used for enlarging up to the allocated
      length.

      \sa _wtBuffer::accept()
  */
  size_t accept(const size_t& s){
    return _wtBuffer::accept(s*sizeof(T)) / sizeof(T);
  }

  /*! \brief Get the size of the buffer
      \return valid length in records

      \sa _wtBuffer::byte_size()
  */
  inline const size_t size( void ) const {
    return length / sizeof(T);
  }

  
  /*! \brief get writeable pointer to buffer
      \return Non-const pointer to buffer

      Will branch() if necessary and may invalid
      any previously obtained pointers to this buffer.
  */
  inline T *writePtr() {
    m_error_t err;
    return get_var( err );
  }

  /*! \brief get writeable pointer to buffer
      \param error will hold error code returned by branch()
      \return Non-const pointer to buffer

      Will branch() if necessary and may invalid
      any previously obtained pointers to this buffer.
  */
  inline T *writePtr( m_error_t& error ) {
    return get_var( error );
  }

  /*! \brief get readable pointer to buffer
      \return const pointer to buffer
  */
  inline const T *readPtr(void) const {
    return get_fix();
  }

  /*! \brief address an item in the array
      \param index of data inside array
      \return const reference to element

      \todo change to proxy-class code to have a read-write
      array.
  */
  inline const T& operator[](const int& index) const{
    return get_fix()[index];
  }

  /*! \brief find the index of an item pointer inside the array
      \param pointer to item in buffer
      \return index of item

      The function interprets the buffer as array of T and calculates
      the index the pointer would have. If the pointer is not
      inside the array -1 is returned, if the pointer is smaller than the
      array start; -2 is returned if the pointer is larger than the
      end of the array.
  */
  inline ssize_t indexOf(const T* p) const{
    ssize_t dist = p - get_fix();
    if(dist < 0) return -1;
    if(static_cast<size_t>(dist) > size()) return -2;
    return dist;
  }

  /*! \brief calculate remaining item space
      \param pointer to item in buffer
      \return number of items following the pointer

      The function returns the number of items following
      the pointer in the valid data area of the buffer.
      If the pointer references the last item, it will
      return 1. Negative returns indicate out of bound
      pointers as described in indexOf().
  */
  inline ssize_t remain(const T* p) const {
    ssize_t dist = indexOf(p);
    if(dist < 0) return dist;
    return size() - dist;
  }

  // translate methods, here sizes are records
  // buffers will be aligned to type (record) boudaries
  // FIXME: Should only apply in case of implementation errors
  inline m_error_t replace(const T *data, const size_t& l){
    size_t rl = length % sizeof(T);
    if(rl) length -= rl;
    return _wtBuffer::replace(data,l*sizeof(T));
  }

  inline m_error_t append(const T *data, const size_t& l){
    size_t rl = length % sizeof(T);
    if(rl) length -= rl;
    return _wtBuffer::append(data,l*sizeof(T));
  }

  inline m_error_t append(const wtBuffer<T>& b){
    size_t rl = length % sizeof(T);
    if(rl) length -= rl;
    rl = b.length / sizeof(T);
    return _wtBuffer::append(b.buf.var,rl*sizeof(T));
  }

  inline m_error_t prepend(const T *data, const size_t& l){
    size_t rl = length % sizeof(T);
    if(rl) length -= rl;
    return _wtBuffer::prepend(data,l*sizeof(T));
  }

  inline m_error_t prepend(const wtBuffer<T>& b){
    size_t rl = length % sizeof(T);
    if(rl) length -= rl;
    rl = b.length / sizeof(T);
    return _wtBuffer::prepend(b.buf.var,rl*sizeof(T));
  }

};

};

#endif // _UTIL_WTBUFFER_H_
