/*
 *
 * Extensible write-through buffer
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: RingBuffer.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  wtBuffer  - an extensible auto-allocation buffer
 *              with write-through capability
 *
 * This defines the values:
 *  DEFAULT_WTBUFFER_CHUNK
 *
 */

#ifndef _TEMPLATES_RINGBUFFER_H_
# define _TEMPLATES_RINGBUFFER_H_

#include <mgrError.h>
#include <stdlib.h>
#include <string.h>

namespace mgr {

  template<class T> class RingBuffer {
  protected:
    T *buffer, *end;
    T *wPtr;
    const T* rPtr;
    bool full;

    inline T* add(T* s, const size_t l){
      s += l;
      if(s >= end) s -= end - buffer;
      return s;
    }
    inline const T* add(const T* s, const size_t l){
      s += l;
      if(s >= end) s -= end - buffer;
      return s;
    }

  public:
    explicit RingBuffer(size_t s) : buffer(NULL), full(false) {
      if(!s) mgrThrowExplain(ERR_PARAM_LEN,"zero sized ring buffer");
      buffer = static_cast<T*>(::malloc(s * sizeof(T)));
      if(!buffer) mgrThrowExplain(ERR_MEM_AVAIL,"allocate ring buffer");
      end = buffer + s;
      wPtr = buffer;
      rPtr = buffer;
    }
    // copy CTOR and assignment yet to think through
    ~RingBuffer() {
      if(buffer) ::free(buffer);
      buffer = NULL;
    }

    inline bool isEmpty() const {
      return ((rPtr == wPtr) && !full);
    }
    inline bool isFull() const {
      return full;
    }
    inline size_t capacity() const {
      return end - buffer;
    }
    size_t remain() const {
      if(full) return 0;
      int l = rPtr - wPtr;
      if(l <= 0) l += capacity();
      return l;
    }
    size_t size() const {
      // int l = capacity() - remain();
      int l = wPtr - rPtr;
      if(!l && !full) return 0;
      if(l <= 0) l += capacity();
      return l;
    }

    m_error_t read(T* dest, size_t& l, bool consume = true){
      if(!dest) return ERR_PARAM_NULL;      
      if(!l) return ERR_NO_ERROR;
      m_error_t err = ERR_NO_ERROR;
      if(l>size()) {
	l = size();
	err = ERR_CANCEL;
      }
      size_t t = l;
      const T* rf = rPtr;
      if(rPtr + l >= end){
	size_t k = end-rPtr;
	if(k){
	  memcpy(dest,rPtr,k*sizeof(T));
	  dest += k;
	  t -= k;
	  rf = buffer;
	}		
      }
      if(t){
	memcpy(dest,rf,t * sizeof(T));
	rf = add(rf,t);
      }
      if(consume) {
	rPtr = rf;
	full = false;
      }

      return err;
    }

    m_error_t drop(size_t& l){
      m_error_t err = ERR_NO_ERROR;
      if(l>size()) {
	l = size();
	wPtr=buffer;
	rPtr=buffer;
	err = ERR_CANCEL;
      } else {
	rPtr = add(rPtr,l);
      }
      full = false;

      return err;
    }

    m_error_t ahead(T* dest, size_t& l, const size_t& offset = 0){
      if(offset >= size()){
	l = 0;
	return ERR_CANCEL;
      }
      const T* rs = rPtr;
      rPtr = add(rPtr,offset);
      m_error_t err = read(dest,l,false);
      rPtr = rs;
      return err;
    }

    void clear(){
      wPtr=buffer;
      rPtr=buffer;
      full = false;
    }      

    m_error_t write(const T* src, size_t& l, bool conserve = true){
      if(!src) return ERR_PARAM_NULL;      
      if(!l) return ERR_NO_ERROR;
      m_error_t err = ERR_NO_ERROR;
      if(l>=remain()) {
	if(conserve) l = remain();
	else {
	  if(l > capacity()){
	    src += l - capacity();
	    memcpy(buffer,src, capacity() * sizeof(T));
	    rPtr = buffer+1;
	    wPtr = buffer;
	    l = size();
	    full = true;
	    return ERR_CANCEL;
	  }
	  src += l - remain();
	  l = remain();
	}
	err = ERR_CANCEL;
	full = true;
      }
      size_t t = l;
      T* wf = wPtr;
      if(wPtr + l >= end){
	size_t k = end-wPtr;
	if(k){
	  memcpy(wPtr,src,k*sizeof(T));
	  src += k;
	  t -= k;
	  wf = buffer;
	}		
      }
      if(t){
	wf = add(wf,t);
	memcpy(wf,src,t * sizeof(T));
      }
      wPtr = wf;

      return err;
    }

    m_error_t push(const T& t){
      if(full) return ERR_CANCEL;
      *wPtr = t;
      if(++wPtr == end) wPtr = buffer;
      if(wPtr == rPtr) full = true;
      return ERR_NO_ERROR;
    }
    const T& pop(m_error_t *err = NULL){
      const T& t = *rPtr;
      if(isEmpty()){
	if(err) *err = ERR_CANCEL;
      } else {
	if(err) *err = ERR_NO_ERROR;
	if(++rPtr == end) rPtr = buffer;
	full = false;
      }
      return t;
    }

  };

};

#endif // _TEMPLATES_RINGBUFFER_H_
