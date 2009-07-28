/*
 *
 * Functions used in higher mathematics and statistics
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * Most algorithms taken from Numerical Recipes
 *
 * $Id: highmath.h,v 1.5 2008-05-15 20:58:24 mgr Exp $
 *
 * This defines the classes:
 *  IterationLimit - an iteration control with divergence watch
 *                   and convergence limits
 *  HighMath       - namespace for all the functions
 *
 * This defines the values:
 *  HighMath::Constants
 *  
 *
 */

#ifndef _MATH_HIGHMATH_H_
#define _MATH_HIGHMATH_H_

#include <math.h>
#include <stdlib.h>
#include <mgrError.h>

using mgr::m_error_t;

namespace HighMath {

class IterationLimit {
public:
  enum enCheckValue { CONTINUE = 0, CONVERGE =  1, DIVERGE = -1 };
  typedef enum enCheckValue CheckValue;

protected:
  size_t maxIter;
  double Epsilon;
  size_t cIter;
  double cVal;
  double cEpsilon;

public:
  IterationLimit(const double& initSum = 0, const size_t& it = 100, const double& eps = 3e-7) 
    : maxIter(it), Epsilon(fabs(eps)), cIter(0), cVal(initSum), cEpsilon(0) {}

  void reset(const double& val = 0){
    cIter = 0;
    cEpsilon = 0;
    cVal = val;
  }

  CheckValue check(const double& val){
    if(cIter++){
      cEpsilon = fabs(cVal - val);
      if(cEpsilon <= fabs(Epsilon * val)) return CONVERGE;
    } 
    cVal = val;
    if(cIter > maxIter) return DIVERGE;
    return CONTINUE;
  }

  CheckValue checkSum(const double& val){
    cVal += val;
    if(fabs(val) <= fabs(cVal * Epsilon)) return CONVERGE;
    if(++cIter > maxIter) return DIVERGE;
    return CONTINUE;
  }

  const double& delta(void){
    return Epsilon;
  }

  const size_t& iteration(void){
    return cIter;
  }

  const double& value(void){
    return cVal;
  }
  
};

class SimpleIntegration {
 public:
  typedef double (*fType)(double);
 protected:
  double s;                 // last integral estimation
  //double (*func)(double);   // function to integrate
  fType func;
  double a,b;               // integration limits
  size_t n;                 // refinement depth

  double trapezInt(double& _s, const size_t _n, 
		   const double _a, const double _b);

 public:
  SimpleIntegration(fType f = NULL, double _a = 0, double _b = 0) 
    : s(0), func(f), a(_a), b(_a), n(0) {}    


  SimpleIntegration& operator=(const SimpleIntegration& si);

  void reset(void){
    s = 0;
    n = 0;
  }
  void reset(double _a, double _b){
    reset();
    a = _a;
    b = _b;
  }

  size_t depth(void){ return n; }

  double step(void){
    return trapezInt(s, ++n, a, b);
  }
};

class Simpson : public SimpleIntegration {
 protected:
  size_t maxIter;
  double Epsilon;

 public:
  Simpson(fType f = NULL, double _a = 0, double _b = 0, 
	  size_t max_it = 20, double eps = 1e-6)
    : SimpleIntegration(f,_a,_b), maxIter(max_it), Epsilon(eps) {};

  Simpson& operator=(const Simpson& si);

  double integrate(m_error_t *err = NULL);
};

class Romberg : public SimpleIntegration {
 protected:
  size_t maxIter;
  double Epsilon;
  size_t K; // extrpolation order

 public:
  Romberg(fType f = NULL, double _a = 0, double _b = 0, size_t _K = 5,
	  size_t max_it = 20, double eps = 1e-6)
    : SimpleIntegration(f,_a,_b), maxIter(max_it), Epsilon(eps), K(_K) {};

  Romberg& operator=(const Romberg& si);

  double integrate(m_error_t *err = NULL);
};

// save space for storing a symmetric matrix with equal diagonal elements
template<typename T> class SymmetricVoigtTensor {
protected:
  T *data;
  size_t dim,ds;

  size_t offset(size_t i, size_t j){
    if(i == j) return 0; // diagonal element
    if(i < j){
      // order pair
      size_t t = i; i = j; j = t;
    }
    if(i >= dim) throw mgr::ERR_INT_BOUND;
    size_t off = i * i;
    off += i;
    off /= 2;
    off = ds - off;
    off += j;
    return off+1;
  }
    

public:
  SymmetricVoigtTensor(const size_t& _dim, const T& diagonal, const T& off) 
    : dim(_dim){
    size_t rdim = dim - 1;
    ds = rdim * rdim;
    ds += rdim;
    ds /= 2;       // (n^2 + n)/2 ordered pairs
    ++ds;          // store diagonal element
    data = (T *)malloc(ds * sizeof(T));
    if(!data) throw mgr::ERR_MEM_AVAIL;
    data[0] = diagonal;
    for(size_t i = 1; i<ds; i++) data[i] = off;
    --ds;          // discard diagonal element for further calculation
  }

  T& operator()(const size_t& i, const size_t& j){
    return data[offset(i,j)];
  }
};

  class Constants {
  public:
    // PI
    static const double PI = 3.1415926535897932384626433832795;
    // 2 / sqrt(PI)
    static const double PI_2bySqrt = 1.1283791670955125738961589031215;
    // 1 / sqrt(2*PI)
    static const double PI_invSqrt2PI = 0.39894228040143267793994605993438;
    // sqrt(2)
    static const double SQRT_2 = 1.4142135623730950488016887242097;
  };

  // ln[Gamma(xx)] for xx > 0
  double lnGamma(double xx);

  // ln(n!)
  double lnFactorial(int n, m_error_t *err = NULL);

  // Returns the binomial coefficient n over k as a floating-point number.
  // The floor function cleans up roundoff error for smaller values of n and k.
  inline double binomial(int n, int k){
    return floor(0.5+exp(lnFactorial(n)-lnFactorial(k)-lnFactorial(n-k)));
  }

  // Returns the value of the beta function B(z,w).
  inline double beta(float z, float w){
    return exp(lnGamma(z)+lnGamma(w)-lnGamma(z+w));
  }

  // Returns the incomplete beta function 
  // Ix(a, b) = 1/beta(a,b) * int_0^x t^(a-1)(1-t)^(b-1)dt
  // I0(a,b)=0, I_1(a,b)=1, Ix(a,b)=1-I_{1-x}(b,a)
  double incompleteBeta(double a, double b, double x, m_error_t *err = NULL);

  // Returns the incomplete gamma function P(a, x) evaluated by its series representation as gamser.
  // Also returns ln G(a) as gln.
  m_error_t incompleteGammaSerial(double *gamser, double a, double x, double *gln);
  // Returns the incomplete gamma function Q(a, x) evaluated by its continued fraction representation 
  // as gammcf. Also returns lnG(a) as gln.
  m_error_t incompleteGammaFractions(double *gammcf, double a, double x, double *gln);
  // Returns the incomplete gamma function P(a, x).
  double incompleteGamma(double a, double x, m_error_t *err = NULL);
  // Returns the complement incomplete gamma function Q(a,x) = 1.0 - P(a, x).
  double incompleteGammaComplement(double a, double x, m_error_t *err = NULL);

  // Standard Gauss distribution
  inline double GaussPhi(const double& x){
    double r = x * x;
    r /= -2.0;
    return Constants::PI_invSqrt2PI * exp(r);
  }

  // Gauss distribution with statistics parameters
  inline double Gauss(const double x, const double sigma, const double mean = 0.0){
    return GaussPhi((x-mean)/sigma) / sigma;
  }

  // Gauss error function: 2/sqrt(PI) * int_0^x exp(-t^2) dt
  inline double erf(double x, m_error_t *err = NULL){
    return (x < 0.0) ? -incompleteGamma(0.5,x*x,err) : incompleteGamma(0.5,x*x,err);
  }
  // complement Gauss error function: 2/sqrt(PI) * int_x^infty exp(-t^2) dt = 1-erf(x)
  inline double erfc(double x, m_error_t *err = NULL){
    return (x < 0.0) ? 1.0+incompleteGamma(0.5,x*x,err) : incompleteGammaComplement(0.5,x*x,err);
  }

  inline double GaussPhiInt(const double& x, m_error_t *err = NULL){
    return erf(x / Constants::SQRT_2, err) / 2.0;
  }

  // Student t Distribution: Density t_r(x)
  double studentDensity(double r,double x);

  // Student t Distribution: Probability A(t,r) = int_-t^t t_r(x) dx
  inline double studentProbability(double r, double t, m_error_t *err = NULL){
    double bix = r + t*t;
    bix = r / bix;
    return 1.0 - incompleteBeta(r / 2.0, 0.5, bix, err);
  }

  // polynomial interpolation
  m_error_t polint(double xa[], double ya[], size_t n, double x, 
		   double *y, double *dy);


  // Integration of descrete monospaced arrays
  double simpson(const double *d, const size_t n, const double h = 1);
};

#endif // _MATH_HIGHMATH_H_
