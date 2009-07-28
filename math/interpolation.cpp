/*
 *
 * Interpolation algorithms
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * Most algorithms taken from Numerical Recipes
 *
 * $Id: interpolation.cpp,v 1.5 2008-05-15 20:58:24 mgr Exp $
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

using namespace mgr;

/*
 * Given arrays xa[1..n] and ya[1..n], and given a value x, 
 * this routine returns a value y, and an error estimate dy. 
 * If P(x) is the polynomial of degree N - 1 such that 
 * P(xa_i) = ya_i, i = 1, ... , n, then the returned value y = P(x).
 *
 */

m_error_t HighMath::polint(double xa[], double ya[], size_t n, double x, 
			   double *y, double *dy){
  size_t i,m,ns=1;
  double den,dif,dift,ho,hp,w;
  double *c,*d;
  dif=fabs(x-xa[1]);
  c=alloc_array(n+1,double);
  d=alloc_array(n+1,double);
  if(!c || !d){
    if(c) ::free(c);
    if(d) ::free(d);
    return ERR_MEM_AVAIL;
  }
  for (i=1;i<=n;i++) { // Here we find the index ns of the closest table entry,
    if ( (dift=fabs(x-xa[i])) < dif) {
      ns=i;
      dif=dift;
    }
    c[i]=ya[i]; // and initialize the tableau of c?s and d?s.
    d[i]=ya[i];
  }
  *y=ya[ns--]; //This is the initial approximation to y.
  for (m=1;m<n;m++) {      // For each column of the tableau,
    for (i=1;i<=n-m;i++) { // we loop over the current c's and d's and update
      ho=xa[i]-x;          // them.
      hp=xa[i+m]-x;
      w=c[i+1]-d[i];
      if ( (den=ho-hp) == 0.0){
	::free(d);
	::free(c);
	return ERR_PARAM_RANG;
      }
      // This error can occur only if two input xa's are 
      // (to within roundoff) identical.
      den=w/den;
      d[i]=hp*den; // Here the c's and d's are updated.
      c[i]=ho*den;
    }
    *y += (*dy=(2*ns < (n-m) ? c[ns+1] : d[ns--]));
    /*
     * After each column in the tableau is completed, we decide, 
     * which correction, c or d, we want to add to our accumulating 
     * value of y, i.e., which path to take through the tableau -- 
     * forking up or down. We do this in such a way as to take the 
     * most "straight line" route through the tableau to its apex, 
     * updating ns accordingly to keep track of where we are. This 
     * route keeps the partial approximations centered (insofar as 
     * ossible) on the target x. The last dy added is thus the error 
     * indication.
     *
     */
  }
  ::free(d);
  ::free(c);
  return ERR_NO_ERROR;
}
