/*
 *
 * String class built on wtBuffer<char>
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: StringBuffer.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  StringBuffer
 *
 * This defines the values:
 *
 */

#ifndef _UTIL_STRINGBUFFER_H_
# define _UTIL_STRINGBUFFER_H_

#include <wtBuffer.h>
#include <string.h>

namespace mgr {

  class StringBuffer : public wtBuffer<char> {
  protected:
    typedef wtBuffer<char> sBuffer;
    char lt(const char *s) const {
      const char *t = readPtr();
      if(!t) return 1;
      char r = 0;
      int i = this->strlen();
      for(;!r && i && *s;--i,++s,++t) r = *s - *t;
      if(r) return r;
      if(i) return -1;
      if(*s) return 1;
      return 0;
    }
    char lt(const StringBuffer& sb) const {
      const char *s = sb.readPtr();
      const char *t = readPtr();
      if(s == t) return 0;
      if(!s) return -1;
      if(!t) return 1;
      int i = size();
      if(sb.strlen() < (size_t)i) i = sb.strlen();
      char r = 0;
      for(;!r && i;--i,++s,++t) r = *s - *t;
      if(r) return r;
      if(!i) return 0;
      if(size() < sb.size()) return 1;
      return -1;
    }

  public:
    StringBuffer(const char *s) : sBuffer(s,::strlen(s)) {}
    StringBuffer(const char *s, const size_t& l) : sBuffer(s,l) {}
    StringBuffer(const StringBuffer& s) : sBuffer(s) {}
    StringBuffer(const sBuffer& s) : sBuffer(s) {}
    StringBuffer(void) : sBuffer() {}
    inline m_error_t replace(const char *s, const size_t& l){
      return wtBuffer<char>::replace(s,l);
    }
    inline m_error_t replace(const char *s){
      return replace(s,::strlen(s));
    }
    class StringBuffer operator=(const StringBuffer& b){
      return static_cast<StringBuffer>(sBuffer::operator=(b));
    }
    class StringBuffer operator=(const char *s){
      this->replace(s,::strlen(s));
      return *this;
    }
    inline size_t strlen() const {
      return size();
    }
    bool operator<(const char *s) const {
      return (lt(s) > 0);
    }
    bool operator<(const StringBuffer& s) const {
      return (lt(s) > 0);
    }
    bool operator>(const char *s) const {
      return (lt(s) > 0);
    }
    bool operator>(const StringBuffer& s) const {
      return (lt(s) > 0);
    }
    bool operator==(const char *s) const {
      return (lt(s) == 0);
    }
    bool operator==(const StringBuffer& s) const {
      return (lt(s) == 0);
    }
    bool operator!=(const char *s) const {
      return (lt(s) != 0);
    }
    bool operator!=(const StringBuffer& s) const {
      return (lt(s) != 0);
    }
    StringBuffer operator+=(const StringBuffer& s) {
      append(s);
      return *this;
    }
    StringBuffer operator+=(const char *s) {
      append(s,::strlen(s));
      return *this;
    }
    const char *cptr(){
      const char *t = readPtr();
      const size_t sl = strlen();
      if(t[sl]){
	if(alloc_size() < sl){
	  append("\0",1);
	  trunc(sl);
	  t = readPtr();
	} else {
	  char *wt = const_cast<char *>(t);
	  wt[sl] = 0;
	}
      }
      return t;
    }      
  };
    
};

#endif // _UTIL_STRINGBUFFER_H_
