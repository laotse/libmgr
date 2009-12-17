/*! \file MemoryRegion.h
    \brief Memory handling

    This file provides classes for raw memory management.

    \version  $Id: MemoryRegion.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
    \author Dr. Lars Hanke
    \date 2007
*/

#ifndef _UTIL_MEMORYREGION_H_
# define _UTIL_MEMORYREGION_H_

#include <unistd.h>
#include <mgrError.h>
#include <RefCounter.h>

namespace mgr {

  class MemoryRegionInterface {
  public:
    MemoryRegionInterface() {}
    MemoryRegionInterface( const void *, const size_t& ) {}
    virtual ~MemoryRegionInterface() {}

    virtual m_error_t replace( const void *, const size_t& ) = 0;
    virtual bool isVoid() const { return true; }
    virtual size_t size() const { return 0; }
  };

  class RawMemoryRegion : public MemoryRegionInterface {
  protected:
    const void *address;
    size_t length;

  public:
    class const_iterator {
    protected:
      const void *p;

    public:
      const_iterator( const void *a = address) : p( a ) {}
      ~const_iterator() {}
      const_iterator( const const_iterator& i ) : p( i.p ) {}
      const_iterator& operator=( const const_iterator& i ) { p = i.p; return *this; }
      void operator*() {}
      ssize_t operator-( const const_iterator& i ) const {
	return static_cast<const char *>(p) - static_cast<const char *>(i.p);
      }
      const_iterator& operator-( const ssize_t& l ) {
	const char *pc = static_cast<const char *>(p) - l;
	p = static_cast<const void *>(pc);
	return *this;
      }
      const_iterator& operator+( const ssize_t& l ) {
	const char *pc = static_cast<const char *>(p) + l;
	p = static_cast<const void *>(pc);
	return *this;
      }
    };

    RawMemoryRegion() : address( NULL ), length( 0 ) {};
    RawMemoryRegion( const void *_adr, const size_t& l ) :
      address( _adr ), length( l ) {}

    RawMemoryRegion& operator=( const RawMemoryRegion& r ){
      address = r.address;
      length = r.length;
    }
    virtual ~RawMemoryRegion() {}

    virtual m_error_t replace(const void *adr, const size_t& l){
      address = adr;
      length=(adr)? l : 0;
      return (adr)? ERR_NO_ERROR : ERR_PARAM_NULL;
    }

    virtual size_t size() const {
      return length;
    }
    bool isVoid() const {
      return (address == NULL);
    }
  };

  class DescriptorFlags {
  protected:
    typedef unsigned char flag_t ;
    enum FlagBits {
      SHARABLE,
      REFERENCE,
      HIGH
    };
    flag_t flags;

    bool check(const enum FlagBits& b) const {
      return (flags & (1 << b)) != 0;
    }

    bool flag(const enum FlagBits& b, const bool& v) {
      if(v){
	flags |= (1 << b);
      } else {
	flags &= ~((flag_t)(1 << b));
      }
      return check(b);
    }

  public:
    DescriptorFlags() : flags(1 << SHARABLE) {}
    ~RCObjectFlags() {}
    RCObjectFlags(const RCObjectFlags& f) : flags(f.flags) {}
    RCObjectFlags &operator=(const RCObjectFlags &f) {
      flags = f.flags;
      return *this;
    }
    bool sharable() const { return check(SHARABLE); }
    bool sharable(bool &init){
      return flag(SHARABLE,init);
    }
    bool isReference() const { return check(REFERENCE);}
    bool isReference(bool& init) {
      return flag(REFERENCE,init);
    }
  };

  template<typename _Alloc = std::allocator<void> > class MemoryRegion {
  private:
    class Descriptor : public RCObject<DescriptorFlags> {
    public:
      RawMemoryRegion region;

      Descriptor() {}
      Descriptor( const RawMemoryRegion& r, bool copy = false ) {
	if(!copy){
	  region = r;
	  Flags.isReference(true);
	} else {
	  region.set(_Alloc::allocate( r.length ), r.length );
	  Flags.isReference(false);
	}
      }
      ~Descriptor() {
	if(!Flags.isReference()){
	  _Alloc::deallocate(r.address, r.length);
	}
      }
    };


  protected:
    RCPtr<Descriptor> Memory;

  public:
    friend class PointerProxy;
    class PointerProxy {
    private:
      MemoryRegion& m;

    public:
      PointerProxy(MemoryRegion *_m) : m(_m) {}
      PointerProxy& operator=( const PointerProxy& rhs );
      PointerProxy& operator=( void * p );

      operator void*() const { return m.Memory->region.ptr(); }

    };

    MemoryRegion() : Memory( new Descriptor ) {}
    MemoryRegion( const MemoryRegion& m ) : Memory( m.Memory ) {}
    MemoryRegion& operator=( const MemoryRegion& m ){
      Memory = m.Memory;
    }
    ~Memory() {}

    const size_t& size() const {
      return Memory->region.size();
    }
    const void *ptr() const {
      return Memory->region.ptr();
    }
  };

};

#endif // _UTIL_MEMORYREGION_H_
