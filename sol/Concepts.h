/*! \file Concepts.h
    \brief Abstract data type concepts: pure virtual classes

    Standard Object Libarary \b libsol.a aims at implementing
    standard data structures by abstract code as used with 
    C-libraries instead of templates as commen with C++ STL.
    There are a couple of major advantages with SOL as compared to STL:
    \li You can have containers for base classes, which will
    contain inherited classes, without adding another level of
    indirection.
    \li Code for a particular container class is created only
    once. This is different to e.g. std::list<int> and 
    std::list<double>, which will produce the std:list base
    code two times.
    \li The base code can be pre-compiled into a libarary.
    The compiler is not burdened with template mangling, which
    drastically enhances compilation resources.

    This file defines pure virtual basic concepts, which are
    used throughout the \b libsol.a. No instantiable classes
    are in this file.

    \version  $Id: Concepts.h,v 1.5 2008-05-15 20:58:24 mgr Exp $
    \author Dr. Lars Hanke
    \date 2007
*/

#ifndef _SOL_CONCEPTS_H_
//! Mark the file as included
# define _SOL_CONCEPTS_H_

#include <mgrError.h>

//! All SOL stuff goes to mgr::sol
namespace mgr { namespace sol {

  /*! \class Cloneable
      \brief Cloneable is the standard interface to the virtual constructor concept

      Whenever polymorphic classes shall be contained in a
      container class, copy and assignment operations must
      be performed on a base class, but must affect the
      real class contained. This requires virtual CTORs, which
      are not supported by C++. Cloneable defines this
      interface.
  */
  class Cloneable {
  protected:
    virtual ~Cloneable() {}

  public:
    //! Copy CTOR
    /*! \return New instance of the real Cloneable
        \note clone() in myClass does something like 
	\code
        static_cast<Cloneable *>(new myClass( *this )).
	\endcode
	The caller must make sure that when casting
	Cloneable pointers to some useful class, a correct
	class is chosen.
    */
    virtual Cloneable *clone() const = 0;
  };

  /*! \class Branchable
      \brief Branchable is the standard interface to the shallow / deep-copy concept

      Complex data structures often take lots of
      resources for deep-copy. Therefore, it is recommended
      to use a shallow-copy concept with reference counted
      data. Turning a shallow-copy into a deep-copy
      is called branching similar to forking a process.
  */
  class Branchable {
  protected:
    virtual ~Branchable() {}

  public:
    //! Ensure we have our own, writeable copy of data
    /*! \return Error code as defined in mgrError.h 
    */
    virtual m_error_t branch() = 0;
  };

};}; // namespace mgr::sol

#endif // _SOL_CONCEPTS_H_
