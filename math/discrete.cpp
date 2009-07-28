/*
 *
 * Basic Integration and Differentiaton algorithms
 * on discrete data sets
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * Most algorithms taken from Numerical Recipes
 *
 * $Id: discrete.cpp,v 1.5 2008-05-15 20:58:24 mgr Exp $
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

//#define DEBUG
#include <mgrDebug.h>

using namespace HighMath;
using namespace mgr;

double HighMath::simpson(const double *d, const size_t n, const double h){
  size_t i = 0;
  double res = 0;

  if(n < 3){
    // oops, we cannot apply simpson
    if(!n) return 0;
    // box approximation
    if(n == 1) return *d * h;
    // trapezoidal approximation
    res = d[0] + d[1];
    res /= 2;
    return res * h;
  }
  if(0 == (n % 2)){
    // even number, add the first interval by trapezoidal rule
    res = d[0] + d[1];
    // scale with respect to final scaling of simpson rule
    res *= 3.0 / 2.0;
    i = 1;
  }

  // initial boundary
  res += d[i++];
  bool odd = true;
  for(;i < n-1;++i){
    res += d[i] * ((odd)? 4 : 2);
    odd = !odd;
  }
  // final boundary
  res += d[i];
  res *= h / 3.0;

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

#define D_ARRAY 50
#define G_WIDTH 10.0

int main(int argc, char *argv[]){
  //m_error_t res;
  size_t tests, errors;
  double ref;

  tests = errors = 0;

  try{
    int fail = 0;
    printf("Test %d: Simpson integration (fine mesh)\n",++tests);
    double dval[D_ARRAY];
    double h = G_WIDTH / (double)(D_ARRAY - 1);
    for(size_t i =0;i<D_ARRAY;++i){
      double x = (double)i * h;
      x -= G_WIDTH / 2.0;
      // x ranges from -5 to 5
      dval[i] = GaussPhi(x);
    }
    double r = HighMath::simpson(dval,D_ARRAY,h);
    ref = 2.0 * GaussPhiInt(G_WIDTH / 2.0);
    double f = fabs(r-ref);
    if(1e-6 >= f){
      printf("???");
    } else {
      ++fail;
      printf("***");
    }
    printf(" %lf, %lf (%lg)\n",ref,r,f);
    if(fail){
      ++errors;
      printf("*** Error: HighMath::Simpson() lacks accuracy\n");
    } else {
      puts("+++ HighMath::Simpson() finished OK!");
    }

    printf("Test %d: Simpson integration (coarse mesh)\n",++tests);
    const double coarseLimit = 1.0; // sigma width has significant values
    const size_t coarseWidth = 4;   // this is the worst configuration, possible
    h = coarseLimit / (double)(coarseWidth - 1);
    for(size_t i =0;i<coarseWidth;++i){
      double x = (double)i * h;
      x -=  coarseLimit / 2.0;
      dval[i] = GaussPhi(x);
    }
    r = HighMath::simpson(dval,coarseWidth,h);
    ref = 2.0 * GaussPhiInt(coarseLimit / 2.0);
    f = fabs(r-ref);
    if(1e-3 >= f){
      printf("???");
    } else {
      ++fail;
      printf("***");
    }
    printf(" %lf, %lf (%lg)\n",ref,r,f);
    if(fail){
      ++errors;
      printf("*** Error: HighMath::Simpson() lacks accuracy\n");
    } else {
      puts("+++ HighMath::Simpson() finished OK!");
    }

    printf("Test %d: Symmetric Voigt Tensor\n",++tests);    
    fail = 0;
    SymmetricVoigtTensor<int> vt(5,1,0);
    for(size_t i=0;i<5;i++){
      for(size_t j=0;j<i;j++){
	vt(i,j) = (i+1) * (j+1);
      }
    }
    for(size_t i=0;i<5;i++){
      for(size_t j=0;j<5;j++){
	if(i == j){
	  if(vt(i,j) != 1){
	    ++fail;
	    printf("*** (%d,%d) %d != %d\n",i,j,vt(i,j),1);
	  }
	} else {
	  int iref = (i+1) * (j+1);
	  if(vt(i,j) != iref){
	    ++fail;
	    printf("*** (%d,%d) %d != %d\n",i,j,vt(i,j),iref);
	  }
	}
      }
    }
    if(fail){
      ++errors;
      printf("*** Error: SymmetricVoigtTesor() has allocation errors\n");
    } else {
      puts("+++ SymmetricVoigtTensor finished OK!");
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
