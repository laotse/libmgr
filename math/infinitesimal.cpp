/*
 *
 * Basic Integration and Differentiaton algorithms
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * Most algorithms taken from Numerical Recipes
 *
 * $Id: infinitesimal.cpp,v 1.5 2008-05-15 20:58:24 mgr Exp $
 *
 * This defines the classes:
 *  SimpleIntegration       - trapezoidal integration with manual steps
 *  Simpson                 - simpson integration to defined accuracy
 *
 * This defines the values:
 *  
 *
 */

#include <highmath.h>
#include <mgrDefines.h>

#define DEBUG
#include <mgrDebug.h>

using namespace HighMath;
using namespace mgr;

/*
 * This routine computes the nth stage of refinement of an 
 * extended trapezoidal rule. func is input as a pointer to 
 * the function to be integrated between limits a and b, also 
 * input. When called with n=1, the routine returns the 
 * crudest estimate of int^b_a f(x)dx. Subsequent calls 
 * with n=2,3,... (in that sequential order) will improve 
 * the accuracy by adding 2n-2 additional interior points.
 *
 */

double SimpleIntegration::trapezInt(double& _s, const size_t _n, 
				    const double _a, const double _b){
  double x,tnm,sum,del;
  size_t it,j;

 if (_n == 1) {
   return (_s=0.5*(_b-_a)*(func(_a)+func(_b)));
 } else {
   for (it=1,j=1;j<_n-1;j++) it <<= 1;
   tnm=it;
   // This is the spacing of the points to be added.
   del=(_b-_a)/tnm; 
   x=_a+0.5*del;
   for (sum=0.0,j=1;j<=it;j++,x+=del) sum += func(x);
   //This replaces s by its refined value.
   _s=0.5*(_s+(_b-_a)*sum/tnm); 
   return _s;
 }
}	

SimpleIntegration& SimpleIntegration::operator=(const SimpleIntegration& si){
  func = si.func;
  s = si.s;
  a = si.a;
  b = si.b;
  n = si.n;

  return *this;
}


/*
 * Returns the integral of the function func from a to b. 
 * The parameters EPS can be set to the desired fractional 
 * accuracy and JMAX so that 2 to the power JMAX-1 is the 
 * maximum allowed number of steps. Integration is performed 
 * by Simpson's rule.
 *
 */

double Simpson::integrate(m_error_t *err){
  size_t j;
  double st,ost=0.0,os=0.0;
  double smallVal = Epsilon * Epsilon;

  if(err) *err = ERR_NO_ERROR;
  for (j=1;j<=maxIter;j++) {
    st=trapezInt(s,j,a,b);
    s=(4.0*st-ost)/3.0; //Compare equation (4.2.4), above.
    smallVal *= Constants::SQRT_2;
    if (j > 5) {        // Avoid spurious early convergence.
      //pdbg("### j=%d s=%lg os=%lg\n",j,s,os);
      if(os > smallVal){
	if (fabs(s-os) < Epsilon*fabs(os)) return s;
      } else if(fabs(s) <= smallVal) return (s=0.0);
    }
    os=s;
    ost=st;
  }
  if(err) *err = ERR_MATH_DIVG;
  return 0.0; 
}

Simpson& Simpson::operator=(const Simpson& si){
  func = si.func;
  a = si.a;
  b = si.b;
  maxIter = si.maxIter;
  Epsilon = si.Epsilon;

  return *this;
}


/*
 * Returns the integral of the function func from a to b. 
 * Integration is performed by Romberg's method of 
 * order 2K, where, e.g., K=2 is Simpson's rule.
 *
 */

double Romberg::integrate(m_error_t *err){
  double dss;
  // These store the successive trapezoidal approximations 
  // and their relative stepsizes.
  double *_s,*h; 
  double smallVal = Epsilon * Epsilon;
  size_t j; 

  // +1 because of FORTRAN array indices
  _s=alloc_array(maxIter+1,double);
  // +2 to avoid checking if j for the final round
  h=alloc_array(maxIter+2,double);
  if(!_s || !h){
    if(_s) ::free(_s);
    if(h) ::free(h);
    if(err) *err = ERR_MEM_AVAIL;
    return 0.0;
  }

  h[1]=1.0;
  _s[0] = 0;
  for (j=1;j<=maxIter;j++) {
    _s[j] = _s[j-1];
    _s[j]=trapezInt(_s[j],j,a,b);
    if (j >= K) {
      polint(&h[j-K],&_s[j-K],K,0.0,&s,&dss);
      //pdbg("### %d(%d) for stepsize in %.2lf-%.2lf %lg: %lf -> %lg\n",
      //   maxIter, j,a,b,h[j],_s[j],s);
      if ((fabs(dss) <= Epsilon*fabs(s)) ||
          (fabs(dss) <= smallVal) && (fabs(s) <= smallVal)){
	:: free(_s);
	:: free(h);
	if(err) *err = ERR_NO_ERROR;
	return s;
      }
    }
    h[j+1]=0.25*h[j];
    smallVal *= Constants::SQRT_2;
    // This is a key step: 
    // The factor is 0.25 even though the stepsize is decreased by 
    // only 0.5. This makes the extrapolation a polynomial in h2 as 
    // allowed by equation (4.2.1), not just a polynomial in h.
  }
  if(err) *err = ERR_MATH_DIVG;
  :: free(_s);
  :: free(h);
  return 0.0; // Never get here.
}

Romberg& Romberg::operator=(const Romberg& si){
  func = si.func;
  a = si.a;
  b = si.b;
  maxIter = si.maxIter;
  Epsilon = si.Epsilon;
  K = si.K;

  return *this;
}


/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <stdio.h>

double myGauss(const double x){
  return GaussPhi(x);
}

const double myStudentDegrees = 12;

double myStudent(const double x){
  return 2.0 * studentDensity(myStudentDegrees,x);
}

int main(int argc, char *argv[]){
  m_error_t res;
  size_t tests, errors;
  double ref;

  tests = errors = 0;

  try{
    int fail = 0;
    printf("Test %d: Simpson integration\n",++tests);
    Simpson sint = Simpson(sin,0,0,25,1.0e-7);
    //Simpson sint = Simpson(sin);
    const double PI = Constants::PI;
    const double b[] = {0, PI/8.0, PI/4.0, PI/2.0, PI, 2.0*PI, 17.0*PI / 8.0};
    for (size_t i=0;i<(sizeof(b) / sizeof(double));++i){
      if(i==5){
	// the 2*PI integral fails Simpson treatment
	//continue;
      }
      ref = 1.0 - cos(b[i]);
      sint.reset(0.0,b[i]);
      double r = sint.integrate(&res);
      double f = (ref)? fabs(r / ref - 1.0) : r;
      if((f < 3e-6) && (res == ERR_NO_ERROR)){
	printf("???");
      } else {
	++fail;
	printf("***");
      }
      printf(" %lf, %lf, %lg (0x%.4x)\n",ref, r, f, (int)res);
    }
    if(fail){
      ++errors;
      printf("*** Error:Simpson::integrate() lacks accuracy\n");
    } else {
      puts("+++ Simpson::integrate() finished OK!");
    }

    printf("Test %d: Romberg integration\n",++tests);
    Romberg rint = Romberg(sin);
    for (size_t i=0;i<(sizeof(b) / sizeof(double));++i){
      ref = 1.0 - cos(b[i]);
      rint.reset(0.0,b[i]);
      double r = rint.integrate(&res);
      double f = (ref)? fabs(r / ref - 1.0) : r;
      if((f < 3e-6) && (res == ERR_NO_ERROR)){
	printf("???");
      } else {
	++fail;
	printf("***");
      }
      printf(" %lf, %lf, %lg (0x%.4x)\n",ref, r, f, (int)res);
    }
    if(fail){
      ++errors;
      printf("*** Error: Romberg::integrate() lacks accuracy\n");
    } else {
      puts("+++ Romberg::integrate() finished OK!");
    }

    printf("Test %d: Romberg Gauss check\n",++tests);
    rint = Romberg(myGauss);
    for(double x = 0; x <= 5; x+= .25){
      rint.reset(0,x);
      double r = rint.integrate(&res);
      ref = GaussPhiInt(x);
      double f = (ref)? fabs(r / ref - 1.0) : r;
      // we compare accuracies 1e-6 and 3e-6, yields 1e-5
      if((f < 1e-5) && (res == ERR_NO_ERROR)){
	printf("???");
      } else {
	++fail;
	printf("***");
      }
      printf("%.2lf: %lf, %lf, %lg (0x%.4x)\n",x, ref, r, f, (int)res);
    }
    if(fail){
      ++errors;
      printf("*** Error: Romberg::integrate() lacks accuracy\n");
    } else {
      puts("+++ Romberg::integrate() finished OK!");
    }

    printf("Test %d: Romberg Student check\n",++tests);
    rint = Romberg(myStudent);
    for(double x = 0; x <= 5; x+= .25){
      rint.reset(0,x);
      double r = rint.integrate(&res);
      ref = studentProbability(myStudentDegrees,x);
      double f = (ref)? fabs(r / ref - 1.0) : r;
      // we compare accuracies 1e-6 and 3e-6, yields 1e-5
      if((f < 1e-5) && (res == ERR_NO_ERROR)){
	printf("???");
      } else {
	++fail;
	printf("***");
      }
      printf("%.2lf: %lf, %lf, %lg (0x%.4x)\n",x, ref, r, f, (int)res);
    }
    if(fail){
      ++errors;
      printf("*** Error: Romberg::integrate() lacks accuracy\n");
    } else {
      puts("+++ Romberg::integrate() finished OK!");
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
