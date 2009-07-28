/*
 *
 * Digest functions - C++ wrapper for openssl
 *
 * (c) 2008 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: bigNum.h,v 1.1 2008-05-15 20:58:24 mgr Exp $
 *
 * This defines the classes:
 *
 * This defines the values:
 *
 */

#ifndef _CYRPTO_DIGEST_H_
# define _CRYPRO_DIGEST_H_

#include <openssl/sha.h>


namespace mgr {

  template<typename Context, const size_t len> class Digest {
  protected:
    Context dig_cont;
    unsigned char Digest[len];
    bool finished;

  public:
    Digest() : finished(false) {}

    m_error_t update(const unsigned char *d, size_t l) {
      mgrThrow(ERR_INT_IMP);
    }

    Digest &operator<<(const char *s){
      m_error_t res = update((const unsigned char *)s,strlen(s));
      if(res != ERR_NO_ERROR) mgrThrow(res);
      return *this;
    }

    size_t size() const {
      return len;
    }

    const unsigned char *digest() {
      if(!finished) mgrThrow(ERR_INT_IMP);
      return Digest;
    }
  };

  typedef Digest<SHA_CTX,SHA_DIGEST_LENGTH> SHA1;
  template<> SHA1::Digest() : finished(false) {
    int res = SHA1_Init(&dig_cont); 
    if(res != 1) 
      mgrThrowFormat(mgr::ERR_INT_COMP,"SHA1_Init() returned %d",res);
  }
  template<> m_error_t SHA1::update(const unsigned char *d, size_t l) {
    int res = SHA1_Update(&dig_cont, d,l);
    if(res != 1) 
      mgrThrowFormat(mgr::ERR_INT_COMP,"SHA1_Update() returned %d",res);
  }    
  template<> const unsigned char *digest() {
    if(!finished){
      int res = SHA1_Final(Digest,&dig_cont);
      finished = true;
      if(res != 1) 
	mgrThrowFormat(mgr::ERR_INT_COMP,"SHA1_Final() returned %d",res);
    }
    return Digest;
  }

};

#endif // _CRYPRO_DIGEST_H_
