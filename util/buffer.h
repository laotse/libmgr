/*
 *
 * Extensible buffer
 *
 * (c) 2005 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: buffer.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  mgrBuffer  - an extensible auto-allocatin buffer for bytes
 *
 * This defines the values:
 *
 */

#ifndef _UTIL_BUFFER_H_
# define _UTIL_BUFFER_H_

#include <unistd.h>
#include <mgrError.h>

class mgrBuffer {
  char *buffer;
  size_t used;
  size_t allocated;
  size_t chunk;

 public:
  m_error_t error;

  mgrBuffer();
  mgrBuffer(size_t);
  ~mgrBuffer();
  char *get(void);
  char *get(size_t);
  char *get(size_t,size_t);
  m_error_t free(void);
  m_error_t trunc(void);
  m_error_t trunc(size_t);
  m_error_t replace(const char *, size_t);
  inline m_error_t replace(const char *s) {return mgrBuffer::replace(s,strlen(s)+1);};
  m_error_t append(const char *, size_t);
  m_error_t prepend(const char *, size_t);  
  m_error_t setChunk(size_t);
  inline size_t size(void) {return mgrBuffer::used;};
  const char *VersionTag(void);
};

#endif // _UTIL_BUFFER_H_
