/*
 *
 * Functions used in higher mathematics and statistics
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * Most algorithms taken from Numerical Recipes
 *
 * $Id: gamma.cpp,v 1.5 2008-05-15 20:58:24 mgr Exp $
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

#include <highmath.h>

using namespace HighMath;
using namespace mgr;

// Gamma-Function Implementation from Numerical Recipes
// Returns the value ln[G(xx)] for xx > 0.
// return precision is only float, but we do not want to convert types too many times
double HighMath::lnGamma(double xx) {
  double x,y,tmp,ser;
  static double cof[6]={76.18009172947146,-86.50532032941677,
			24.01409824083091,-1.231739572450155,
			0.1208650973866179e-2,-0.5395239384953e-5};
  int j;
  y=x=xx;
  tmp=x+5.5;
  tmp -= (x+0.5)*log(tmp);
  ser=1.000000000190015;
  for (j=0;j<=5;j++) ser += cof[j]/++y;
  return -tmp+log(2.5066282746310005*ser/x);
}

// ln(n!)
// caches in lookup-table
double HighMath::lnFactorial(int n, m_error_t *err){
  static double a[101];
  
  if (n < 0){
    if(err) *err = ERR_PARAM_RANG;
    return -1;
  }
  if(err) *err = ERR_NO_ERROR;
  if (n <= 1) return 0.0;
  if (n <= 100) return (a[n])? a[n] : (a[n]=lnGamma(n+1.0)); 
  else return lnGamma(n+1.0);
}

// Returns the incomplete gamma function P(a, x) evaluated by its series representation as gamser.
// Also returns ln G(a) as gln.
m_error_t HighMath::incompleteGammaSerial(double *gamser, double a, double x, double *gln){
  double del,ap;

  *gln=lnGamma(a);
  if (x <= 0.0) {
    *gamser=0.0;
    return (x < 0.0)? ERR_PARAM_RANG : ERR_NO_ERROR;
  } else {
    ap=a;
    del=1.0/a;
    IterationLimit il(del);
    IterationLimit::CheckValue state;
    do {
      ++ap;
      del *= x/ap;
    } while(IterationLimit::CONTINUE == (state = il.checkSum(del)));
    if(state != IterationLimit::CONVERGE) return ERR_MATH_DIVG;
    *gamser=il.value()*exp(-x+a*log(x)-(*gln));
    return ERR_NO_ERROR;
  }
}

// Returns the incomplete gamma function Q(a, x) 
// evaluated by its continued fraction representation 
// as gammcf. Also returns lnG(a) as gln.
m_error_t HighMath::incompleteGammaFractions(double *gammcf, double a, double x, double *gln){
  double an,b,c,d,del,h;
  const double FPMIN = 1e-30;
  *gln=lnGamma(a);
  // Set up for evaluating continued fraction 
  // by modified Lentz's method (.....2) with b0 = 0.
  b=x+1.0-a; 
  c=1.0/FPMIN;
  d=1.0/b;
  h=d;
  IterationLimit il;
  IterationLimit::CheckValue state;
  //for (i=1;i<=ITMAX;i++) { // Iterate to convergence.
  do {    
    // an = -i*(i-a);
    an = a - (double)(il.iteration() + 1);
    an *= il.iteration() + 1;
    b += 2.0;
    d=an*d+b;
    if (fabs(d) < FPMIN) d=FPMIN;
    c=b+an/c;
    if (fabs(c) < FPMIN) c=FPMIN;
    d=1.0/d;
    del=d*c;
    h *= del;
  } while(IterationLimit::CONTINUE == (state = il.check(del - 1.0)));
  if(state != IterationLimit::CONVERGE) return ERR_MATH_DIVG;
  *gammcf=exp(-x+a*log(x)-(*gln))*h;
  return ERR_NO_ERROR;
}

// Returns the incomplete gamma function P(a, x).
double HighMath::incompleteGamma(double a, double x, m_error_t *err){
  double res,gln;

  if (x < 0.0 || a <= 0.0){
    if(err) *err = ERR_PARAM_RANG;
    return -1;
  }
  if (x < (a+1.0)) { 
    // Use the series representation.
    m_error_t ierr = incompleteGammaSerial(&res,a,x,&gln);
    if(err) *err = ierr;
    return res;
  } else { 
    // Use the continued fraction representation and take its complement.
    m_error_t ierr = incompleteGammaFractions(&res,a,x,&gln);
    if(err) *err = ierr;
    return 1.0-res; 
  }
}

// Returns the complement incomplete gamma function Q(a,x) = 1.0 - P(a, x).
double HighMath::incompleteGammaComplement(double a, double x, m_error_t *err){
  double res,gln;

  if ((x < 0.0) || (a <= 0.0)){
    if(err) *err = ERR_PARAM_RANG;
    return -1;
  }
  if (x < (a+1.0)) { 
    // Use the series representation.
    m_error_t ierr = incompleteGammaSerial(&res,a,x,&gln);
    if(err) *err = ierr;
    return 1.0 - res;
  } else { 
    // Use the continued fraction representation and take its complement.
    m_error_t ierr = incompleteGammaFractions(&res,a,x,&gln);
    if(err) *err = ierr;
    return res; 
  }
}


// Used by incompleteBeta(): 
// Evaluates continued fraction for incomplete beta function by 
// modified Lentz's method (.....2).
static m_error_t betacf(double& h, const double& a, const double& b, const double& x) { 
  size_t m,m2;
  double aa,c,d,del,qab,qam,qap;
  const double FPMIN = 1e-30;

  qab=a+b;    // These q's will be used in factors that occur
  qap=a+1.0;  // in the coefficients (6.4.6).
  qam=a-1.0;
  c=1.0;      // First step of Lentz's method.
  d=1.0-qab*x/qap;
  if (fabs(d) < FPMIN) d=FPMIN;
  d=1.0/d;
  h=d;
  IterationLimit il(0,100,3e-7);
  IterationLimit::CheckValue state;
  //  for (m=1;m<=MAXIT;m++) {
  do {
    m = il.iteration() + 1;
    m2=2*m;
    aa=m*(b-m)*x/((qam+m2)*(a+m2));
    d=1.0+aa*d; // One step (the even one) of the recurrence.
    if (fabs(d) < FPMIN) d=FPMIN;
    c=1.0+aa/c;
    if (fabs(c) < FPMIN) c=FPMIN;
    d=1.0/d;
    h *= d*c;
    aa = -(a+m)*(qab+m)*x/((a+m2)*(qap+m2));
    d=1.0+aa*d; // Next step of the recurrence (the odd one).
    if (fabs(d) < FPMIN) d=FPMIN;
    c=1.0+aa/c;
    if (fabs(c) < FPMIN) c=FPMIN;
    d=1.0/d;
    del=d*c;
    h *= del;
  } while(IterationLimit::CONTINUE == (state = il.check(del - 1.0)));
  if(state != IterationLimit::CONVERGE) return ERR_MATH_DIVG;
  return ERR_NO_ERROR;
}


// Returns the incomplete beta function 
// Ix(a, b) = 1/beta(a,b) * int_0^x t^(a-1)(1-t)^(b-1)dt
// I0(a,b)=0, I_1(a,b)=1, Ix(a,b)=1-I_{1-x}(b,a)
double HighMath::incompleteBeta(double a, double b, double x, m_error_t *err){
  double bt,cfb;

  if (x < 0.0 || x > 1.0) {
    if(err) *err = ERR_PARAM_RANG;
  }
  if (x == 0.0 || x == 1.0) bt=0.0;
  else //Factors in front of the continued fraction.
    bt=exp(lnGamma(a+b)-lnGamma(a)-lnGamma(b)+a*log(x)+b*log(1.0-x));
  if (x < (a+1.0)/(a+b+2.0)) {
    //Use continued fraction directly.
    m_error_t ierr = betacf(cfb,a,b,x);
    if(err) *err = ierr;
    return bt*cfb/a;
  } else {
    // Use continued fraction after making the symmetry
    // transformation
    m_error_t ierr = betacf(cfb,b,a,1.0-x);
    if(err) *err = ierr;
    return 1.0-bt*cfb/b;
  }
}

// Student t density t_r(x)
double HighMath::studentDensity(double r,double x){
  // gamma((n+1)/2)./sqrt(n*pi)./gamma(n/2).*(1+x.^2./n).^(-(n+1)/2);
  double res = x * x;
  res /= r;
  res = pow(1.0 + res, (r + 1.0) / -2.0);
  res /= sqrt(r * Constants::PI);
  res *= exp(lnGamma((r + 1.0)/2.0) - lnGamma(r / 2.0));
  
  return res;
}

/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <stdio.h>

int main(int argc, char *argv[]){
  m_error_t res;
  size_t tests, errors;
  double ref;

  tests = errors = 0;

  try{

    printf("Test %d: Highmath::Gamma\n",++tests);
    ref = 1;
    int fail = 0;
    for(int i=1;i<25;++i){
      ref *= i;
      double r = exp(lnGamma(i+1));
      double f = fabs(r/ref - 1.0);
      if(f >= 1e-10){
	printf("*** %d! = %.0lf, %.1lf, %lg\n",i,ref,r,f);
	++fail;
      } else {
	printf("??? %d! = %.0lf, %.1lf, %lg\n",i,ref,r,f);
      }
    }
    if(fail){
      ++errors;
      printf("*** Error:lnGamma() lacks accuracy\n");
    } else {
      puts("+++ HighMath::lnGamma() finished OK!");
    }

    printf("Test %d: Highmath::GaussIntegral\n",++tests);
    fail = 0;
    // values listed in Bronstein
    const double erftab[] = { 0, 0.1915, 0.3413, 0.4332, 0.4772, 
			      0.4937903, 0.4986501, 0.4997674,
			      0.4999683, 0.4999966, 0.4999997 };
    int i = 0;
    for(double d=0; d<=5.0;d+=0.5){
      ref = erftab[i++];
      double r = GaussPhiInt(d, &res);
      double f = fabs(r - ref);
      // 1e-4 is accuracy of bronstein table
      if(f > 1e-4){
	++fail;
	printf("*** ");
      } else {
	printf("??? ");
      }
      printf("Phi_0(%.1lf) = %lf (%lf) %.lg%%\n",d, r, ref, 100.0*f);
      if(res != ERR_NO_ERROR){
	++fail;
	printf("*** Error:erf() returned 0x%.4x\n",(int)res);
      }
    }
    if(fail){
      ++errors;
      printf("*** Error:erf() lacks accuracy\n");
    } else {
      puts("+++ HighMath::erf() finished OK!");
    }

    printf("Test %d: Highmath::binomial\n",++tests);
    fail = 0;
    double pascal[2][15];
    pascal[0][0] = 1.0;
    for(int i=1;i<=15;++i){
      pascal[i%2][0] = 1.0;
      int j = 1;
      for(;j<(i+1)/2;++j)
	pascal[i%2][j] = pascal[(i+1)%2][j-1] + pascal[(i+1)%2][j];
      for(;j<i;++j)
	pascal[i%2][j] = pascal[i%2][i-j-1];
      printf("??? %d: ",i);
      for(j=0;j<i;++j){
	if(pascal[i%2][j] != binomial(i-1,j)){
	  printf("*%.1lf!=",binomial(i-1,j));
	  ++fail;
	}
	printf("%.0lf ",pascal[i%2][j]);
      }
      printf("\n");
    }	
    if(fail){
      ++errors;
      printf("*** Error:binomial() lacks accuracy\n");
    } else {
      puts("+++ HighMath::binomial() finished OK!");
    }

    printf("\n%d tests completed with %d errors.\n",tests,errors);
    // printf("Used version: %s\n",HighMath::VersionTag());
    
    return 0;  
  }
  // m_error_t caught as int
  catch(int err){
    printf("*** Exception thrown: 0x%.4x\n",err);
  }
}

#endif // TEST
