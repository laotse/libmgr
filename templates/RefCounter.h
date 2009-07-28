/*
 *
 * Extensible write-through buffer
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: RefCounter.h,v 1.6 2008-05-17 19:33:28 mgr Exp $
 *
 * This defines the classes:
 *  wtBuffer  - an extensible auto-allocation buffer
 *              with write-through capability
 *
 * This defines the values:
 *  DEFAULT_WTBUFFER_CHUNK
 *
 */

#ifndef _TEMPLATES_REFCOUNTER_H_
# define _TEMPLATES_REFCOUNTER_H_

#include <mgrError.h>

namespace mgr {

  /*! \class RCObject
      \brief Reference counter base class for inheriting

      This is the base class for reference counted content.
      The interface object will not contain the content,
      but point to a reference counted object ideally using
      an RCPtr object.

      This class shall be used as 
      \code class myContent : public RCObject; \endcode

      \note RCObject and RCPtr are taken from Scott Meyers
      <tt>More effective C++</tt>.
  */
  class RCObject {
  protected:
    //! Number of instances using this content
    size_t refCount;
    //! If false, this copy is exclusive and cannot be shared
    bool sharable;
    
  public:
    //! Creation of new contents
    RCObject() : refCount(0), sharable(true) {}
    //! Deep-Copy of contents
    RCObject(const RCObject&) : refCount(0), sharable(true) {}
    //! Assignemt of RCObject is empty
    RCObject& operator=(const RCObject&) { return *this; }
    //! empty DTOR
    virtual ~RCObject() {}

    //! Lock this instance
    inline void lock() { ++refCount; }
    //! Release this instance and delete, if used no longer
    inline void release() {
      if(--refCount == 0) delete this;
    }
    //! Mark as unsharable, i.e. deep-copy on next assignment
    /*! \sa RCPtr::init() */
    inline void taint() { sharable = false; }
    //! Check sharable property
    inline bool isSharable() const { return sharable; }
    //! Check whether we have more than one client
    inline bool isShared() const { return refCount > 1; }
  };

  /*! \class RCPtr
      \brief Smart pointer for accessing RCObject

      This class does all the bookkeeping necessary
      to operate a RCObject. Make this class a
      member of you interface class using the 
      reference counted contents.
      
      \note taint() must be maintained by the
      interface class, since RCPtr cannot
      know about oprations done to the content.

      This class shall be used as 
      \code 
      class myContent : public RCObject;

      class myInterface {
        protected:
          RCPtr<myContent> Content;

        public:
          myInterface& myInterface( const myInterface& i ) : Content( i.Content );
	  myInterface& operator=( const myInterface& i ) {
	    Content = i.Content;
	    \/\/ do whatever there's to do
          }
          myContent& get() { return *Content; }
      };
      \endcode

      Consider to inherit from mgr::sol::Branchable for reference counted
      interface classes: 
      \code class myInterface : public mgr::sol::Branchable; \endcode

      \note RCObject and RCPtr are taken from Scott Meyers
      <tt>More effective C++</tt>.
  */
  template<class T> class RCPtr {
  protected:
    //! Pointer to reference counted content
    T *ptr;

    /*! \brief Common initialisations for all CTOR

        init() clones the contents, unless they are
	sharable (\sa RCObject::taint()). If the
	content is sharable, it simply acquires
	another lock.

        \note init() throws with new, but this is okay, 
        because it's used in CTORs only
    */
    inline void init(){
      if(ptr == NULL) return;
      if(!(ptr->isSharable())) ptr = new T(*ptr);
      ptr->lock();
    }

    //! Internals of branch(), in case there is something to branch
    /*! branch() is divided into a quick inlining part
        and the working horse here.
    */
    m_error_t _branch() throw() {
      T *np = NULL;
      try {
	np = new T(*ptr);
      } 
      catch( Exception& e ){
#ifdef DEBUG_LOG
	fprintf(DEBUG_LOG,e.what());
#endif
	return e.getCause();
      }
      catch(...) {
	return ERR_MEM_AVAIL;
      }
      np->lock();
      if(ptr) ptr->release();
      ptr = np;
      return ERR_NO_ERROR;
    }

  public:
    //! CTOR both empty and initializing
    RCPtr(T *p = NULL) : ptr(p) { init(); }
    //! Copy CTOR
    RCPtr(const RCPtr& r) : ptr(r.ptr) { init(); }
    //! DTOR releases contents
    /*! \sa RCObject::release() */
    ~RCPtr() { if(ptr) ptr->release(); }
    /*! \brief Assignement operator
  
        The assignment operator copies
	the contents by the rules stated
	in init(). Since init() may throw,
	the operator cathces the exception,
	reverts the object, and rethrows.
    */
    RCPtr& operator=(const RCPtr& r){
      if(ptr == r.ptr) return *this;
      T* old = ptr;
      ptr = r.ptr;
      try {
	init();
      } catch(...) {
	ptr = old;
	throw;
      }
      if(old) old->release();

      return *this;
    }

    /*! \brief Check whether two objects have the same pointee
        \param r RCPtr to compare to
        \return true, if the objects have the same pointee
    */
    bool operator==(const RCPtr& r) const {
      return ptr == r.ptr;
    }

    /*! \brief Swap pointees
        \param r RCPtr to swap contents with
    */
    inline void swap(RCPtr& r){
      T *old = ptr;
      ptr = r.ptr;
      r.ptr = old;
    }

    /*! \brief Check whether the RCPtr has a valid pointee
        \return true, if there is no valid pointee
    */
    bool isNull() const {
      return ptr == NULL;
    }

    /*! \brief Support mgr::sol::Branchable
        \return Error code as defined in mgrError.h
      
        This method assures that the current instance of the
	contents is owned exclusively by this RCPtr. This
	is particularly interesting for implementing 
	<em>copy on write</em>. Call branch() before granting
	a write pointer.

	branch() is distributed in a do nothing check for inlining
	and the real branch functionality as protected member
	_branch(), which hopefully is compiled to some object
	file.

	The method cathces all throws and converts them to
	error codes.

	\note branch()'ed contents are still sharable. If you
	want exclusive contents use RCObject::taint() \b after
	branch(), or consider using an object without
	reference counting.
     */
    inline m_error_t branch() throw() {
      return (ptr->isShared())? _branch() : ERR_NO_ERROR;
    }
      
    //! Pointer interface dereferencing
    T* operator->() const { return ptr; }
    //! Pointer interface dereferencing
    /*! \note Dereferencing throws ERR_PARAM_NULL, if ptr is NULL */
    T& operator*() const { if(!ptr) mgrThrow(ERR_PARAM_NULL); return *ptr; }
  };

  /*! \class RCIPtr
      \brief Add reference counting to standard objects
      \param T Class ob object to be reference counted

      This is the synthesis of RCObject and RCPtr in
      a single template. It can be used to make any
      class a reference counted object. It introduces
      another level of indirection, however.

      The content class is implemented as nested,
      private class Counter. The RCIPtr wraps
      the real content class and is not required
      to be a member somewhere, it is the class
      you're using in the end.
  */
  template<class T> class RCIPtr {
  private:
    //! The reference counted object container
    struct Counter : public RCObject {
      //! Self-destruct for bookkeeping
      ~Counter() { delete ptr; }
      //! The pointer to real data
      T* ptr;
    };
    //! The pointer to the object container with the pointer to the data
    Counter *counter;
    
    /*! \brief Common initialisations for all CTOR

        init() clones the contents, unless they are
	sharable (\sa RCObject::taint()). If the
	content is sharable, it simply acquires
	another lock.

        \note init() throws with new, but this is okay, 
        because it's used in CTORs only
    */
    void init() {
      if(!counter->isSharable()){
	T *old = counter->ptr;
	counter = new Counter;
	counter->ptr = old? new T(*old) : NULL;
      }
      counter->lock();
    }

  public:
    //! Standard CTOR and initializing CTOR
    RCIPtr(T* p = NULL) : counter(new Counter) {
      counter->ptr = p;
      init();
    }
    //! Copy CTOR creating new reference, unless clone is required
    RCIPtr(const RCIPtr& r) : counter(r.counter) { init(); }
    //! DTOR releasing reference
    ~RCIPtr() { counter->release(); }
    //! Assigment operator
    /*! \note This one has no roll-back and can throw */
    RCIPtr& operator=(const RCIPtr& r){
      if(counter == r.counter) return *this;
      counter->release();
      counter=r.counter();
      init();
      return *this;
    }
    //! Pointer dereferencing interface
    T* operator->() const { return counter->ptr; }
    //! Pointer dereferencing interface
    T& operator*() const { return *(counter->ptr); }
  };

  /*! \class DynamicSingleton
      \brief Class with 0 or 1 instance throughout the entire application
      \param Derived Class for Singleton instance

      This class defines the standard template, which can be
      used by any constructable class for enforcing at
      most a single instance to exist at any time during
      the program flow. This is handy for mutices, intra-process
      communication, or process data like signal handlers.

      The Singleton is dynamic, because it may also have 0
      instances. The object of class derived is constructed
      and destroyed as it is needed.
      
      The Singleton is not constructed using a standard
      CTOR, but using the static member function create().

      \code
      typedef DynamicSingleton<myClass> mySingle;
      myClass *instance = mySingle::create();
      \endcode

      Don't forget to implement the static variables 
      somewhere in the code or get linkage errors:

      \code
      unsigned int mySingle::Counter = 0;
      mySingle::Object_t *mySingle::Object = NULL;
      \endcode
  */
  template<typename Derived> class DynamicSingleton {
  private:
    //! The type of the single Object
    typedef Derived Object_t;
    //! Unique reference counter to Singleton
    static unsigned int Counter;
    //! Unique pointer to Singleton object
    static Object_t *Object;

    //! You must not construct
    DynamicSingleton() {}
    //! DTOR is reducing reference count and delete if unused
    ~DynamicSingleton() {
      if(Counter && --Counter) return;
      // we're in the DTOR, i.e. we're going to be deleted
      // delete Object;
      if(Object) delete Object;
      Object = NULL;
      Counter = 0;
    }
    //! You must not copy construct
    DynamicSingleton( const DynamicSingleton& );

  public:
    //! This is the only CTOR allowed
    static Derived  *create() {
      if(Object == NULL) {
	Object = new Object_t;
	Counter = 1;
      }
      return Object;
    }

    Derived *operator->() const { return Object; }
    Derived& operator*() const { return *Object; }
  };

  /*! \class DSingleton
      \brief Class with 0 or 1 instance throughout the entire application
      \param Load Class for Singleton instance

      This class defines the standard template, which can be
      used by any constructable class for enforcing at
      most a single instance to exist at any time during
      the program flow. This is handy for mutices, intra-process
      communication, or process data like signal handlers.

      The Singleton is dynamic, because it may also have 0
      instances. The object of class derived is constructed
      and destroyed as it is needed.

      The DSingleton can be used interchangably with a pointer
      to the payload class. It is constructed and destroyed
      just as any other variable, e.g. could be regarded as
      a smart-pointer. The actual singleton is a private
      sub-class.

      \code
      // instead of
      // myClass myVal;
      // myClass *myPtr = &myVal;
      DSingleton<myClass> myPtr;
      \endcode
      
      Don't forget to implement the static variables 
      somewhere in the code or get linkage errors:

      \code
      typedef DSingleton<myClass> mySingle;
      template<>
      unsigned int mySingle::Singleton::Counter = 0;
      template<>
      mySingle::Object_t *mySingle::Singleton::Object = NULL;
      \endcode
  */
  template<typename Load> class DSingleton {
  private:
    //! The embedded singleton
    class Singleton {
    private:      
      //! Reference counter
      static unsigned int Counter;  
      //! Single instace pointer
      static Load *Object;

      //! Cannot be created
      Singleton() {}
      //! Cannot be destroyed
      ~Singleton() {}

    public:
      //! Static factory instead of CTOR
      static Load *create() {
	if(Object == NULL){
	  Object = new Load;
	  Counter = 1;
	} else ++Counter;

	return Object;
      }
      //! Static release hook instead of DTOR
      static void release() {
	if(Counter && --Counter) return;
	if(Object) delete Object;
	Object = NULL;
	Counter = 0;
      }
    };
    
  protected:
    Load *Object;

  public:
    typedef Load Object_t;

    DSingleton() {
      Object = Singleton::create();
    }
    ~DSingleton() {
      Singleton::release();
    }

    DSingleton& operator=(const DSingleton&){
      if(Object == NULL) 
	Object = Singleton::create();
      return *this;
    }
    DSingleton(const DSingleton&){
      Object = Singleton::create();
    }

    Load *operator->() const { return Object; }
    Load& operator*() const { return *Object; }
    operator Load *() const { return Object; }
    
  };

  /*! \class DSingleton
      \brief Class with 0 or 1 instance throughout the entire application
      \param Load Class for Singleton instance

      This class defines the standard template, which can be
      used by any non-constructable class for enforcing at
      most a single instance to exist at any time during
      the program flow. This is pretty handy for all kinds
      of handles returned from standard C libraries, where
      encapsulation of the maintenance structure itself
      is unavailable and using DSingleton would lead to
      unnecessary indirection.

      The Singleton is dynamic, because it may also have 0
      instances. The object of class derived is constructed
      and destroyed as it is needed.

      The DHSingleton can be used interchangably with the
      payload class. It is constructed and destroyed
      just as any other variable, e.g. could be regarded as
      as a simple variable. The actual singleton is a private
      sub-class.

      \code
      // instead of
      // myClass myVal;
      DHSingleton<myClass> myPtr;
      \endcode
      
      Don't forget to implement the static variables 
      somewhere in the code or get linkage errors:

      \code
      typedef DHSingleton<myClass> mySingle;
      template<>
      unsigned int mySingle::Singleton::Counter = 0;
      template<>
      mySingle::Object_t mySingle::Singleton::Object = NULL;
      \endcode

      Note that construction and destruction is performed by
      static methods in the Singleton. These are implemented
      to just throw. So for using the class template, it must
      be specilialized, i.e.:
      
      \code
      template<> void mySingle::Singleton::init() {
        // put a value to Object
      }
      template<> void mySingle::Singleton::destroy() {
        // free resources associated with the current value in Object
      }
      \endcode
  */
  template<typename Load> class DHSingleton {
  private:
    //! The embedded singleton
    class Singleton {
    private:      
      //! Reference counter
      static unsigned int Counter;  
      //! Single instace pointer
      static Load Object;

      //! Cannot be created
      Singleton() {}
      //! Cannot be destroyed
      ~Singleton() {}

    protected:
      static void init() { mgrThrow(ERR_INT_IMP); }
      static void destroy() { mgrThrow(ERR_INT_IMP); }	

    public:
      //! Static factory instead of CTOR
      static Load &create() {
	if(!Counter){
	  init();
	  Counter = 1;
	} else ++Counter;

	return Object;
      }
      //! Static release hook instead of DTOR
      static void release() {
	if(!Counter || --Counter) return;
	destroy();
      }
      static Load &get() { return Object; }
    };

  public:
    typedef Load Object_t;

    DHSingleton() {
      Singleton::create();
    }
    ~DHSingleton() {
      Singleton::release();
    }

    DHSingleton& operator=(const DHSingleton&){
      return *this;
    }
    DHSingleton(const DHSingleton&){
      Singleton::create();
    }

    operator Load &() const { return Singleton::get(); }
    
  };

};

#endif // _TEMPLATES_REFCOUNTER_H_
