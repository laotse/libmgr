/*
 *
 * Universal Output Stream for wtBuffers
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: wtBufferDump.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  StreamDump - Interface Class
 *
 * This defines the values:
 *
 */

#ifndef _UTIL_WTBUFFERDUMP_H_
# define _UTIL_WTBUFFERDUMP_H_

#include <StreamDump.h>
#include <wtBuffer.h>

namespace mgr {

class BufferDump : public StreamDump {
protected:
  wtBuffer<char> buf;

public:
  BufferDump(size_t chunk = 1024){
    buf.Chunk(chunk);
  }
  virtual ~BufferDump() {}

  inline const wtBuffer<char>& get(void) { return buf; }

  // no overload for printf
  // virtual m_error_t printf(size_t *out, const char *fmt, ...);
  // the rest is trivial ...
  virtual m_error_t write(const void *data, size_t *s) {    
    return buf.append((const char *)data, *s);
  }
  virtual m_error_t flush(void) { return ERR_NO_ERROR; }
  virtual bool valid(void) const { return true; }
  virtual m_error_t close(void) { return buf.trunc(0); }
  virtual m_error_t putchar(const void *data) {
    return buf.append((const char *)data, 1);
  }
  inline m_error_t putchar(char c){
    return buf.append(&c, 1);
  }

};

}; // namespace mgr

#endif // _UTIL_WTBUFFERDUMP_H_
