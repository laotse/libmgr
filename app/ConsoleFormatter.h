/*
 *
 * ConsoleFormatter to print formatted text to a stream
 * used primarily to print help texts
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: ConsoleFormatter.h,v 1.5 2008-05-15 20:58:24 mgr Exp $
 *
 * This defines the classes:
 *
 * This defines the values:
 *
 */

/*! \file ConsoleFormatter.h
    \brief Print formatted text to a stream

    The classes in this file help to print text to the console with
    rudimentary formatting. The module is especially designed to
    write help texts and the like using a command line driven application.
    The author of the text should not be bothered with the deatils of
    the text layout on a specific console device.

    \version  $Id: ConsoleFormatter.h,v 1.5 2008-05-15 20:58:24 mgr Exp $
    \author Dr. Lars Hanke
    \date 2004-2007
*/


#ifndef _APP_CONSOLE_FORMATTER_H_
#define _APP_CONSOLE_FORMATTER_H_

#include <mgrError.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

namespace mgr {
  /*! \class ConsoleFormatter
      \brief Formatted output to a file

      This class can be roughly compared to a fstream.

      \todo Move from std:string to some _wtBuffer based implementation,
      or just allocate width+1.
  */
  class ConsoleFormatter {
  protected:
    std::string indent_str;  //!< indendation string filled with ' '
    const char *cIndent;     //!< pointer into indent_str yielding the current indent
    size_t indent;           //!< Current indent depth
    size_t width;            //!< Screen width
    FILE *fil;               //!< Stream to write output to
    m_error_t error;         //!< Internal error variable like errno

    //! Init fil with a dup()'ed stream
    /*! \param f Stream to write data to
        \return Error code as defined in mgrError.h

        The function performs dup() on the fd of
	the stream passed and attaches a new stream to
	the dup()'ed fd. Therefore the file fil used inside
	this class is independent from the parameter f.
    */
    m_error_t fDup(FILE *f){
      if(fil) fclose(fil);
      fil = NULL;
      if(!f) return ERR_PARAM_NULL;
      int fd = fileno(f);
      if(fd < 0) return ERR_FILE_LIBC;
      fil = fdopen(fd,"a");
      if(!fil) return ERR_FILE_OPEN;
      return ERR_NO_ERROR;
    }

    //! Create new indendation string and set pointer
    m_error_t makeIndent();

  public:
    //! CTOR creates new object
    /*! \param f Stream to write data to
        \param w Width of screen in characters (default: 80)
	\param i Initial indent in characters (default: 0)

	The stream will be dup()'ed internally, so it's safe to
	fclose() the stream externally, while the object is still
	in scope.
    */
    ConsoleFormatter(FILE *f, const size_t& w = 80, const size_t& i = 0)
      : cIndent(NULL), indent(i), width(w), fil(NULL), error(ERR_NO_ERROR) {
      error = fDup(f);
    }
    
    //! Copy CTOR
    ConsoleFormatter(const ConsoleFormatter& si) :
      indent_str(si.indent_str), indent(si.indent), width(si.width) {
      error = makeIndent();
      if(error != ERR_NO_ERROR){
	fil = NULL;
      } else error = fDup(si.fil);      
    }

    //! DTOR closes file
    ~ConsoleFormatter() {
      if(fil){
	fclose(fil);
	fil = NULL;
      }
    }

    //! Assignment operator
    ConsoleFormatter& operator=(const ConsoleFormatter& si){
      indent_str.clear();
      
      indent = si.indent;
      width = si.width;
      error = makeIndent();
      if(error != ERR_NO_ERROR){
	fil = NULL;
      } else error = fDup(si.fil);      
      error = fDup(si.fil);
      
      return *this;
    }

    //! Get current indent depth
    inline const size_t& Indent() {
      return indent;
    }

    //! Set new indent width
    /*! \param newIndent Width in characters to indent text
        \return Width the text will be actually indented

	The parameter newIndent is wrapped by width. cIndent
	is set void for lazy evalution in the printing
	methods.
    */
    inline const size_t& Indent(const size_t& newIndent) {
      if(newIndent >= width){
	indent = newIndent % width;
      } else {
	indent  = newIndent;
      }
      cIndent = NULL;
      return indent;
    }

    //! Read and reset internal error code
    inline m_error_t readError() {
      m_error_t err = error;
      error = ERR_NO_ERROR;
      return err;
    }

    //! Print text to formatter
    m_error_t print(const char *s);
    //! \overload m_error_t print(const char *s);
    inline m_error_t print(const std::string& s){
      return print(s.c_str());
    }

    //! Print text to formatter
    /*! \param s Text to print

        This is the C++ fstream style operator << interface
	to m_error_t print(const char *). In case of an error
	the error code will be saved internally and is available
	through readError().

	\note A successful operation will not clear an error
	code previously stored.
    */
    inline ConsoleFormatter& operator<<(const char *s){
      m_error_t err = print(s);
      if(error == ERR_NO_ERROR) error = err;
      return *this;
    }

    //! \overload ConsoleFormatter& operator<<(const char *s)
    inline ConsoleFormatter& operator<<(const std::string& s){
      return operator<<(s.c_str());
    }

    //! Print a line-feed '\\n'
    inline m_error_t lineFeed(){
      if(!fil) return ERR_PARAM_NULL;
      if(EOF == fputc('\n',fil)) return ERR_FILE_WRITE;
      return ERR_NO_ERROR;
    }

    //! Version information string
    const char *VersionTag() const;
  };
};


#endif // _APP_CONSOLE_FORMATTER_H_
