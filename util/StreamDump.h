/*
 *
 * Universal Output Stream Implementation
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: StreamDump.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  StreamDump - Interface Class
 *
 * This defines the values:
 *
 */

#ifndef _UTIL_STREAMDUMP_H_
# define _UTIL_STREAMDUMP_H_

#include <mgrError.h>
#include <unistd.h>
#include <stdio.h>

#if defined(putchar)
#undef putchar
#endif

/*! \file StreamDump.h
    \brief Alternative File Interface

    In particular for debugging purposes often some
    file output with special formatting is required. StreamDump
    offers an interface for such output and allows easy change
    of representation, e.g. ASCII or Hex, by changing the
    StreamDump implementation.

    It may be used as an alternative to std::streams if error
    reporting through m_error_t is favored instead of throwing.
    But StreamDump was never intended as a drop-in replacement
    of streams and uses different notation. In particular
    operator<<() makes no sense, if we want to return the
    error code.

    \todo The undefining of putchar follows into the file including
    StreamDump.h. Search for a better solution.
    \author Dr. Lars Hanke
    \date 2006-2007
*/


namespace mgr {

  /*! \class StreamDump
      \brief Interface of an alternative stream like class

      This class implements much of the idea of std::stream,
      but relies on error codes instead of throwing. The functions
      resemble the c-library <stdio.h> style functions.

      StreamDump itself is only an interface. Most functions
      are pure virtual. It supplies a printf() style method
      for format string treatment, which uses the overloaded
      write() method for output. It should be quite generic
      and handy in all classes implementing StreamDump.
  */
class StreamDump {
protected:
  virtual ~StreamDump() {}

public:
  /*! \brief Write a data region to stream
      \param data Start of data region
      \param s Length of data in region
      \retval s Length of data actually written
      \return Error code as defined in mgrError.h
  */
  virtual m_error_t write(const void *data, size_t *s) = 0;

  /*! \brief Write a single character to stream
      \param data Address of the character
      \return Error code as defined in mgrError.h

      A character may be something wiered. Multibyte
      unicode characters are the more natural
      choice, why no char type was taken and
      a pointer is used instead.
  */
  virtual m_error_t putchar(const void *data) = 0;

  /*! \brief Flush buffers related to stream
      \return Error code as defined in mgrError.h

      This includes all OS and glibc related buffers.
  */
  virtual m_error_t flush(void) = 0;

  /*! \brief close file
      \return Error code as defined in mgrError.h
  */
  virtual m_error_t close(void) = 0;

  /*! \brief Check, if the StreamDump is ready to be written
      \return true, if StreamDump is accepting data
  */
  virtual bool valid(void) const = 0;

  /*! \brief The fprintf() method
      \param fmt Format string followed by arguments
      \retval out Number of bytes written to StreamDump
      \return Error code as defined in mgrError.h

      The printf() method is actually implemented in
      StreamDump and draws on write() for output. It should
      be pretty generic for all c-string related streams and
      thus would not require overloading.
  */
  virtual m_error_t printf(size_t *out, const char *fmt, ...);
};

  /*! \class FileDump
      \brief Implementation of StreamDump for ordinary files
  */
class FileDump : public StreamDump {
protected:
  FILE *f;   //!< glibc file handle

public:
  //! CTOR from existing file handle
  /*! \param fil stdio file handle

      The file is dup()'ed. So the file handle
      passed may be closed, without affecting
      the FileDump.
  */
  FileDump(FILE *fil);

  //! CTOR from file name
  FileDump(const char *name);

  //! DTOR closing file
  virtual ~FileDump(){ close(); }

  //! Overloaded to use fprintf()
  virtual m_error_t printf(size_t *out, const char *fmt, ...);

  //! Overloaded to use fwrite()
  virtual m_error_t write(const void *data, size_t *s);

  //! fflush() essentially
  virtual m_error_t flush(void);

  //! Check, if we're ready for data
  virtual bool valid(void) const { return (f != NULL); }

  //! fclose() essentially
  virtual m_error_t close(void);

  //! Interface to fputc()
  virtual m_error_t putchar(const void *data){
    if(!f || !data) return ERR_PARAM_NULL;
    if(EOF == ::fputc(*((const char *)data),f)) return ERR_FILE_WRITE;
    return ERR_NO_ERROR;
  }

  //! Overloaded to have standard interface available
  /*! \param c Charactzer to write to FileDump

      \note Using this variant is handy, but breaks
      compatibility, when replacing the StreamDump
      implementation.
  */
  inline m_error_t putchar(const char& c){
    if(!f) return ERR_PARAM_NULL;
    if(EOF == ::fputc(c,f)) return ERR_FILE_WRITE;
    return ERR_NO_ERROR;
  }
    
  //! Version string
  const char * VersionTag(void) const;
};

}; // namespace mgr

#endif // _UTIL_STREAMDUMP_H_
