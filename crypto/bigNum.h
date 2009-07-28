/*
 *
 * Large Integer arithemtics - C++ wrapper for openssl
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

#ifndef _CYRPTO_BIGNUM_H_
# define _CRYPRO_BIGNUM_H_

#include <RefCounter.h>
#include <string>
#include <openssl/evp.h>
#include <openssl/bn.h>

namespace mgr {

  /*! \brief Encapsulates BN_CTX *

      The DHSingleton ensures that there is
      at most one BN_CTX allocated at all times,
      but that there is none allocated, unless
      a BN_CTX is in use.

      Since a lot of primitives, e.g. multiplication,
      use such contexts, it may be a good idea to
      to just create an instance of BigNumCtx before
      starting large calculations. As long as it remains
      in scope, no new BN_CTX is allocated.

      \code
      {
        BigNumCtx Context;

	// do lots of calculations on single context
      }
      // here BN_CTX is freed again
      \endcode
  */
  typedef DHSingleton<BN_CTX *> BigNumCtx;
  template<> void BigNumCtx::Singleton::init() {
    Object = BN_CTX_new();
    //puts("### new context created!");
    if(!Object) mgrThrow(ERR_MEM_AVAIL);
  }
  template<> void BigNumCtx::Singleton::destroy() {
    if(Object) BN_CTX_free(Object);
    //puts("### context purged!");
    Object = NULL;
  }

  class _BigNumber : public RCObject {
  protected:
    BIGNUM Number;

  public:
    _BigNumber() {
      BN_init(&Number);
    }

    ~_BigNumber() {
      BN_clear(&Number);
    }

    _BigNumber(const _BigNumber& bn) {
      BN_init(&Number);
      if(NULL == BN_copy(&Number, &(bn.Number)))
	mgrThrow(ERR_MEM_AVAIL);
    }
    _BigNumber(const long v){
      unsigned long q = (v < 0)? -v : v;
      BN_init(&Number);
      int res = BN_set_word(&Number, q);
      if(v<0) BN_set_negative(&Number,1);
      if(res != 1) mgrThrow(ERR_MATH);
    }
    _BigNumber(BIGNUM& bn){
      BN_init(&Number);
      BN_swap(&Number, &bn);
    }
	
    long value() {
      int n = Number.neg;
      if(n) Number.neg = 0;
      unsigned long q = BN_get_word(&Number);
      Number.neg = n;
      if(q > 0x80000000) mgrThrow(ERR_PARAM_RANG);
      return n? -(long)q : (long)q;
    }

    int cmp(const _BigNumber& bn) const {
      return BN_cmp(&(this->Number),&(bn.Number));
    }

    const BIGNUM& val() { return Number; }

    std::string decimal() const {
      char *s = BN_bn2dec(&Number);
      if(!s) mgrThrow(ERR_MEM_AVAIL);
      std::string st(s);
      OPENSSL_free(s);
      return st;
    }

    std::string hex() const {
      char *s = BN_bn2hex(&Number);
      if(!s) mgrThrow(ERR_MEM_AVAIL);
      std::string st(s);
      OPENSSL_free(s);
      return st;
    }

    m_error_t mul(const _BigNumber& a) {
      BigNumCtx ctx;
      int r = BN_mul(&Number, &Number, &(a.Number), ctx);
      if(r == 1) return ERR_NO_ERROR;
      return ERR_MATH;
    }

    void negate() {
      if(Number.neg) Number.neg = 0;
      else Number.neg = 1;
    }

    m_error_t mul(long a){
      unsigned long b = (a>0)? a: -a;
      int r = BN_mul_word(&Number,b);
      if(a<0) negate();
      if(r == 1) return ERR_NO_ERROR;
      return ERR_MATH;
    }      

    m_error_t div(const _BigNumber& a, _BigNumber *rm = NULL) {
      BigNumCtx ctx;
      BN_CTX_start(ctx);
      BIGNUM *res = BN_CTX_get(ctx);
      BIGNUM *rem = BN_CTX_get(ctx);
      if(!res || !rem){
	BN_CTX_end(ctx);
	return ERR_MATH;
      }
      int r = BN_div(res, rem, &Number, &(a.Number), ctx);
      if(rm) BN_swap(&(rm->Number), rem);
      BN_swap(&Number, res);
      BN_CTX_end(ctx);
      if(r == 1) return ERR_NO_ERROR;
      return ERR_MATH;
    }

    m_error_t div(long a, long *rm = NULL){
      unsigned long b = (a>0)? a: -a;
      if(a<0) negate();
      if(b == 0) return ERR_MATH_DIVZ;
      long rem = BN_div_word(&Number, b);
      if(rm) *rm = rem;
      return ERR_NO_ERROR;
    }

    m_error_t add(const _BigNumber& a) {
      int r = BN_add(&Number, &Number, &(a.Number));
      if(r == 1) return ERR_NO_ERROR;
      return ERR_MATH;
    }

    m_error_t add(long a){
      int r = 1;
      if(a<0) {
	r = BN_sub_word(&Number,-a);
      } else if(a > 0) {
	r = BN_add_word(&Number,a);
      }
      if(r == 1) return ERR_NO_ERROR;
      return ERR_MATH;
    }      

    m_error_t sub(const _BigNumber& a) {
      int r = BN_sub(&Number, &Number, &(a.Number));
      if(r == 1) return ERR_NO_ERROR;
      return ERR_MATH;
    }

    //! Version information string
    const char * VersionTag(void) const;
  };

  class BigNumber {
  protected:
    RCPtr<_BigNumber> Number;

  public:
    BigNumber() : Number( new _BigNumber ) {}
    BigNumber( const long& v) : Number( new _BigNumber( v ) ) {}
    BigNumber( const BigNumber& bn) : Number( bn.Number ) {}
    BigNumber& operator=( const BigNumber& bn) {
      Number = bn.Number;
      return *this;
    }

    operator long() const { return Number->value(); }

    bool operator>(const BigNumber& bn) const {
      if(Number == bn.Number) return false;
      return (Number->cmp(*bn.Number)) > 0;
    }
    bool operator<(const BigNumber& bn) const {
      if(Number == bn.Number) return false;
      return (Number->cmp(*bn.Number)) < 0;
    }
    bool operator==(const BigNumber& bn) const {
      if(Number == bn.Number) return true;
      return (Number->cmp(*bn.Number)) == 0;
    }
    bool operator!=(const BigNumber& bn) const {
      return !operator==(bn);
    }
    bool operator>=(const BigNumber& bn) const {
      if(Number == bn.Number) return true;
      return (Number->cmp(*bn.Number)) >= 0;
    }
    bool operator<=(const BigNumber& bn) const {
      if(Number == bn.Number) return true;
      return (Number->cmp(*bn.Number)) <= 0;
    }
    
    BigNumber& operator*=(const BigNumber& bn) {
      m_error_t res = Number.branch();
      if(res != ERR_NO_ERROR) mgrThrow(res);
      res = Number->mul(*bn.Number);
      if(res != ERR_NO_ERROR) mgrThrow(res);
      return *this;
    }

    BigNumber operator*(const BigNumber& bn) const {
      BigNumber r(*this);
      return r *= bn;
    }

    BigNumber& operator*=(long a) {
      m_error_t res = Number.branch();
      if(res != ERR_NO_ERROR) mgrThrow(res);
      res = Number->mul(a);
      if(res != ERR_NO_ERROR) mgrThrow(res);
      return *this;
    }

    BigNumber operator*(long a) const {
      BigNumber r(*this);
      return r *= a;
    }

    BigNumber& operator/=(const BigNumber& bn){
      m_error_t res = Number.branch();
      if(res != ERR_NO_ERROR) mgrThrow(res);
      res = Number->div(*bn.Number);
      if(res != ERR_NO_ERROR) mgrThrow(res);
      return *this;
    }

    BigNumber operator/(const BigNumber& bn) const {
      BigNumber r(*this);
      return r /= bn;
    }

    BigNumber& operator/=(long a) {
      m_error_t res = Number.branch();
      if(res != ERR_NO_ERROR) mgrThrow(res);
      res = Number->div(a);
      if(res != ERR_NO_ERROR) mgrThrow(res);
      return *this;
    }

    BigNumber operator/(long a) const {
      BigNumber r(*this);
      return r /= a;
    }

    BigNumber& divmod(const BigNumber& bn, BigNumber& rem) {
      m_error_t res = Number.branch();
      if(res != ERR_NO_ERROR) mgrThrow(res);
      res = Number->div(*bn.Number, &(*rem.Number));
      if(res != ERR_NO_ERROR) mgrThrow(res);
      return *this;
    }            

    BigNumber& divmod(long a, long& rem) {
      m_error_t res = Number.branch();
      if(res != ERR_NO_ERROR) mgrThrow(res);
      res = Number->div(a, &rem);
      if(res != ERR_NO_ERROR) mgrThrow(res);
      return *this;
    }            

    BigNumber& swap(BigNumber& bn){
      Number.swap(bn.Number);
      return *this;
    }

    BigNumber& operator%=(const BigNumber& bn){
      m_error_t res = Number.branch();
      if(res != ERR_NO_ERROR) mgrThrow(res);
      BigNumber rem;
      res = Number->div(*bn.Number, &(*rem.Number));
      if(res != ERR_NO_ERROR) mgrThrow(res);
      swap(rem);
      return *this;
    }

    BigNumber operator%(const BigNumber& bn) const {
      BigNumber r(*this);
      return r %= bn;
    }

    BigNumber& operator%=(long a) {
      m_error_t res = Number.branch();
      if(res != ERR_NO_ERROR) mgrThrow(res);
      long rem;
      divmod(a,rem);
      operator=(BigNumber(rem));
      return *this;
    }      

    long operator%(long a) const {
      BigNumber r(*this);
      long rem;
      r.divmod(a,rem);
      return rem;
    }

    BigNumber& operator+=(const BigNumber& bn) {
      m_error_t res = Number.branch();
      if(res != ERR_NO_ERROR) mgrThrow(res);
      res = Number->add(*bn.Number);
      if(res != ERR_NO_ERROR) mgrThrow(res);
      return *this;
    }
      
    BigNumber operator+(const BigNumber& bn) const {
      BigNumber r(*this);
      return r += bn;
    }

    BigNumber& operator+=(long a) {
      m_error_t res = Number.branch();
      if(res != ERR_NO_ERROR) mgrThrow(res);
      res = Number->add(a);
      if(res != ERR_NO_ERROR) mgrThrow(res);
      return *this;
    }

    BigNumber operator+(long a) const {
      BigNumber r(*this);
      return r += a;
    }

    BigNumber& operator-=(const BigNumber& bn) {
      m_error_t res = Number.branch();
      if(res != ERR_NO_ERROR) mgrThrow(res);
      res = Number->sub(*bn.Number);
      if(res != ERR_NO_ERROR) mgrThrow(res);
      return *this;
    }
      
    BigNumber operator-(const BigNumber& bn) const {
      BigNumber r(*this);
      return r -= bn;
    }

    BigNumber& operator-=(long a) {
      m_error_t res = Number.branch();
      if(res != ERR_NO_ERROR) mgrThrow(res);
      res = Number->add(-a);
      if(res != ERR_NO_ERROR) mgrThrow(res);
      return *this;
    }

    BigNumber operator-(long a) const {
      BigNumber r(*this);
      return r -= a;
    }


    std::string toDecimal() const {
      return Number->decimal();
    }

    std::string toHex() const {
      return Number->hex();
    }

    long value() const {
      return Number->value();
    }

    const char * VersionTag(void) const {
      return Number->VersionTag();
    }
  };
};

#endif // _CYRPTO_BIGNUM_H_
