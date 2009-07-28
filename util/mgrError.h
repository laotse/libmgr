/*
 *
 * Error tracking and reporting system
 *
 * (c) 2005 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: mgrError.h,v 1.4 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *
 * This defines the values:
 *  error codes
 *
 */

/*! \file mgrError.h
    \brief Error definition and handling

    This file defines the error codes used throughout the entire library.
    It furthermore defines exception classes and some helper macros.

    \author Dr. Lars Hanke
    \date 2005-2007
*/

#ifndef _UTIL_MGR_ERROR_H_
#define _UTIL_MGR_ERROR_H_

#include <exception>
#include <new>
#include <stdlib.h>
#include <stdarg.h>

namespace mgr {
  //! The error codes used throughout this library
  enum ErrorCodes {
    ERR_NO_ERROR  = 0x000, //!< No error, code OK
    ERR_CANCEL    = 0x001, //!< No error, just cancel action
    ERR_FILE      = 0x100, //!< File related errors  
    ERR_FILE_OPEN = 0x101, //!< Error openening file
    ERR_FILE_CLOSE= 0x102, //!< Error closing file
    ERR_FILE_READ = 0x103, //!< Error reading file
    ERR_FILE_WRITE= 0x104, //!< Error writing file
    ERR_FILE_STAT = 0x105, //!< Error accessing file, e.g. get file length
    ERR_FILE_MKDIR= 0x106, //!< Error creating directory
    ERR_FILE_ISDIR= 0x107, //!< Expected a regular file, but got directory
    ERR_FILE_END  = 0x108, //!< Premature end of file
    ERR_FILE_CONT = 0x109, //!< File not completely read
    ERR_FILE_LIBC = 0x10a, //!< libc internals, e.g. fileno() fails
    ERR_FILE_SOCK = 0x10b, //!< socket creation failed
    ERR_FILE_LOCK = 0x10c, //!< File is locked
    ERR_FILE_EXEC = 0x10d, //!< execve() failure
    ERR_MEM       = 0x200, //!< Memory and resource related errors
    ERR_MEM_AVAIL = 0x201, //!< memory could not be allocated
    ERR_MEM_FORK  = 0x202, //!< fork() fails
    ERR_MEM_SIG   = 0x203, //!< signal() fails
    ERR_MEM_TIME  = 0x204, //!< timer resource not available
    ERR_PARAM     = 0x300, //!< Parameter passing errors
    ERR_PARAM_NULL= 0x301, //!< NULL pointer passed
    ERR_PARAM_RANG= 0x302, //!< parameter out of range
    ERR_PARAM_OPT = 0x303, //!< option value is not of required type
    ERR_PARAM_LEN = 0x304, //!< Length parameter of unsuitable value
    ERR_PARAM_SEL = 0x305, //!< selection parameter (enum type) not supported
    ERR_PARAM_KEY = 0x306, //!< key or keyword unknown
    ERR_PARAM_RUN = 0x307, //!< runaway argument, missing delimiter
    ERR_PARAM_XNUL= 0x308, //!< NULL pointer expected
    ERR_PARAM_UDEF= 0x309, //!< required parameter is missing
    ERR_PARAM_TYP = 0x30a, //!< parameter object is not of required type
    ERR_PARAM_UNIQ= 0x30b, //!< parameter is required to be unique, but is not
    ERR_PARAM_END = 0x30c, //!< requested parameter missing
    ERR_PARAM_LCK = 0x30d, //!< parameter cannot be changed currently, ro
    ERR_INT       = 0x400, //!< Internal, i.e. programming error
    ERR_INT_BOUND = 0x401, //!< Array boundary out of range
    ERR_INT_RANG  = 0x402, //!< Internal enum out of range
    ERR_INT_COMP  = 0x403, //!< Internal compatibility problem 
    ERR_INT_STATE = 0x404, //!< Impossible condition encountered
    ERR_INT_DATA  = 0x405, //!< Internal data structure corrupt
    ERR_INT_IMP   = 0x406, //!< Not yet implemented
    ERR_INT_SEQ   = 0x407, //!< Method not available in current state of object
    ERR_PARS      = 0x500, //!< Parser error
    ERR_PARS_STX  = 0x501, //!< Syntax error
    ERR_PARS_END  = 0x502, //!< unexpected end of input
    ERR_CLS       = 0x600, //!< Class related errors
    ERR_CLS_CREATE= 0x601, //!< Class constructor failed
    ERR_MATH      = 0x700, //!< Errors with special maths
    ERR_MATH_DIVG = 0x701, //!< approximation diverges
    ERR_MATH_DIVZ = 0x702, //!< division by zero

    ERR_MAJORCODE =~0x0ff //!< Mask out subcode of error
  };
  typedef enum ErrorCodes m_error_t; //!< Error code type
};

/*! \brief Assign error code to an optional pointer
    \param x variable of type m_error_t *
    \param e error code macro without leading ERR_

    This macro is intended for functions, which do not return
    an error code, but have an error pointer argument allowed
    as NULL pointer. The macro assigns the error code to
    the pointer, if it is non-NULL.
*/
#define mgrErrPut(x,e) if(x) *(x)=ERR_ ## e

/*! \brief Assign error code to an optional pointer and return
    \param r return code of any type
    \param x variable of type m_error_t *
    \param e error code macro without leading ERR_

    This macro is intended for functions, which do not return
    an error code, but have an error pointer argument allowed
    as NULL pointer. The macro assigns the error code to
    the pointer, if it is non-NULL.
    
    After assigning the error code it returns r.

    \sa mgrErrPut()
*/
#define mgrErrRet(r,x,e) do{mgrErrPut(x,e); return r;}while(0)

/*! \brief Assign error code to an optional pointer and return
    \param r return code of any type
    \param x variable of type m_error_t *
    \param e error code

    This is identical to mgrErrRet(), but can return any type
    of code or assign an internal variable to the error pointer. There
    is no ERR_ prepended to whatever is passed.

    This macro is intended for functions, which do not return
    an error code, but have an error pointer argument allowed
    as NULL pointer. The macro assigns the error code to
    the pointer, if it is non-NULL.
    
    After assigning the error code it returns r.

    \sa mgrErrRet()
*/
#define mgrErrRetV(r,x,e) do{if(x) *(x)=e; return r;}while(0)

/*! \brief Tolerate ERR_CANCEL
    \param r variable holding an error code
    \return true, if code is minor or no error

    This macro checks the error code for ERR_NO_ERROR
    or ERR_CANCEL and returns true.

    \note The argument is evaluated multiple times, so use variables
    as arguments. No pre- or postincrement and no function returns.
*/
#define mgrErrMinor(r) (((r) == ERR_NO_ERROR) || ((r) == ERR_CANCEL))

namespace mgr {

  /*! \class Exception
      \brief This Exception contains verbose information

      The paradigma of this library is that errors are handled where
      they occur. Throwing is capitulation and should be limited to
      situations, where an error code cannot be notified or the error
      is definitively lethal. The latter case is a programming bug - period!
      The former case, e.g. in CTORs, a weakness in the design of C++.

      This exception class reports cause and position of the error faciliating
      debugging. This is the design goal and purpose of this class, which 
      inherits from std::exception.
  */
  class Exception : public std::exception {
  protected:
    const m_error_t cause;     //!< Error code - cause of error
    const char * const fil;    //!< Source file where the exception was thrown
    const size_t line;         //!< Line number of the exception instatiation
    const char *Explain;       //!< Explanatory text
    char *text;                //!< Final text for output i what()
    char *ExpBuffer;           //!< Buffer, if explanatory text is created dynamically

    /*! \brief formatting function printf() style
        \param fmt format string
        \param args arguments for format string
	
	This function is used to format explanatory text, if
	parametrized text is passed.
    */
    void format(const char * const fmt, va_list args);

  public:
    /*! \brief Constructor for minimal Exception
        \param c error code causing the exception
	\param f file name, should be set ti __FILE__
	\param l line number, should be set to __LINE__

	Creates an exception without explanatory text.

	\sa mgrThrow()
    */
    Exception(const m_error_t c, const char * const f, const size_t l)
      : cause(c), fil(f), line(l), Explain(NULL), text(NULL), ExpBuffer(NULL) {}

    /*! \brief Constructor for minimal Exception
        \param c error code causing the exception
	\param f file name, should be set ti __FILE__
	\param l line number, should be set to __LINE__
	\param fmt a printf() style format string

	Creates an exception with explanatory text. If fmt contains
	format parameters, the construction of the final string
	may fail. This will of course not throw, but a standard
	error message will be displayed. A constant explanation
	string will not fail.

	\sa mgrThrowExplain() mgrThrowFormat()
    */
    Exception(const m_error_t c, const char * const f, const size_t l, const char * const fmt, ... )
      : cause(c), fil(f), line(l), text(NULL), ExpBuffer(NULL){
      va_list args;
      va_start(args, fmt);
      format(fmt,args);
      va_end(args);
    }

    /*! \brief Destructor

        Does all cleaning up, e.g. buffer for explanatory text.
    */
    virtual ~Exception() throw() {
      if(text) ::free(text);
      text = NULL;
      if(ExpBuffer) ::free(ExpBuffer);
      ExpBuffer = NULL;
    }
    
    /*! \brief Set explanatory text
        \param t Text to set for error code explanation
	\sa what()
    */
    void explain(const char *t) throw() {
      Explain = t;
    }

    //! Version information string
    static const char *VersionTag(void);
    
    /*! \brief Get explanation string
      
        This is the overloaded function of std::exception,
	which returns the explanatory text eventually formatted
	with all parameters for display of the error cause.
    */
    virtual const char *what() const throw();

    //! \brief Get throwing code
    m_error_t getCause() const throw() { return cause; }
  };

};

//! Throw error code without special explanation
/*! __FILE__ and __LINE__ are set automatically
    \note Since a template function does not allow
    default parameters and would furthermore insert
    the line in this file as location, the mgrThrow
    family is implemented as macros.
*/
#define mgrThrow(x) throw mgr::Exception(x,__FILE__,__LINE__)

//! Throw error code with constant explanation
/*! __FILE__ and __LINE are set automatically */
#define mgrThrowExplain(x,e) throw mgr::Exception(x,__FILE__,__LINE__,e)

//! Throw error code with format string and parameters for explanation
/*! __FILE__ and __LINE are set automatically */
#define mgrThrowFormat(x,f,...) throw mgr::Exception(x,__FILE__,__LINE__,f, __VA_ARGS__)

//! Create error code without special explanation
/*! \return Exception

    __FILE__ and __LINE are set automatically. The exception
    is not thrown.
*/
#define mgrException(e,x) mgr::Exception e(x,__FILE__,__LINE__)

#endif // _UTIL_MGR_ERROR_H_
