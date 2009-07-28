/*
 *
 * Universal Output Stream - Filter for HexDump generation
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: HexDump.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  HexDump - Fileter for StreamDump to create Hex output
 *
 * This defines the values:
 *
 */

#ifndef _UTIL_HEXDUMP_H_
# define _UTIL_HEXDUMP_H_

#include <StreamDump.h>
#include <wtBuffer.h>

/*! \file HexDump.h
    \brief A StreamDump plumberer

    This class implements a StreamDump translates its
    input to Hex and outputs this new character
    stream to yet another StreamDump.

    \author Dr. Lars Hanke
    \date 2006-2007
*/


namespace mgr {

  /*! \class HexDump
      \brief A StreamDump plumberer
  
      This class implements a StreamDump translates its
      input to Hex and outputs this new character
      stream to yet another StreamDump.
  */
class HexDump : public StreamDump {
protected:
  StreamDump *out;     //!< StreamDump to write to
  bool textmode;       //!< If textmode is true, do not convert to Hex
  char *prefix;        //!< Text to prefix before each new line
  wtBuffer<char> wb;   //!< working buffer for the current line
  size_t linepos;      //!< "cursor" position

public:
  HexDump(StreamDump *o, bool mode = false);
  virtual ~HexDump() {}

  virtual m_error_t write(const void *data, size_t *s);
  virtual m_error_t putchar(const void *d);
  // these are trivial
  virtual m_error_t flush(void) { return out->flush(); }
  virtual m_error_t close(void) { return out->close(); }
  virtual bool valid(void) const { return out->valid(); }
  //virtual m_error_t printf(size_t *out, const char *fmt, ...);

  m_error_t lineFeed(void);

  // some interfaces for convenience
  inline m_error_t write(const wtBuffer<char>& b){
    size_t s = b.byte_size();
    return write(b.readPtr(), &s);
  }

  // just like printf, but force text mode
  m_error_t textf(size_t *os, const char *fmt, ...);

  const char * VersionTag(void) const;
};

}; // namespace mgr

#endif // _UTIL_HEXDUMP_H_
