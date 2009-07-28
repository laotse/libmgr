/**********************************************************
 *
 * C - Source file identification Header
 *
 * Copyright (c) 1994 by MGR - Software, Asgard
 * written 1994 by Lars Hanke
 *
 **********************************************************
 *
 * PROJECT: Wavemeter
 *  MODULE: misc.dll
 *    FILE: units.c
 *
 **********************************************************
 *
 *   AUTHOR: Lars Hanke
 *  REVISOR: Lars Hanke
 * REVISION: 0.9
 *     DATE: 14/09/94
 *
 **********************************************************
 *
 * DESCRIPTION
 *      This code module supplies some miscellaneous
 *      routines for parsing and conversion of physical
 *      property strings and values.
 *
 * EXPORTS
 *      ml_cut_value()
 *      ml_trim()
 *      LibMain()
 *
 * IMPORTS
 *      units.h
 *      buffers.h
 *
 *      struct List CustomMem
 *
 * NOTES
 *      This is my old ml_trim() workhorse from units.c
 *      rapidly converted to a c++ class
 *      Comments may apply to the original implementation
 *      The property part, i.e. unit converter has been stripped off
 *
 **********************************************************
 *
 * HISTORY
 *  - 27/09/94  corrected a severe error that causes a general protection
 *              fault with misuses of ml_parse_property()
 *  - 20/09/94  added ml_parse_property_save() and ml_explain_error()
 *            * hunted down a bug in the isxxx functions and found
 *              some severe disregards of ANSI standards in QCWIN
 *              it now seems to work, when the /J (unsigned char)
 *              compiler option is set
 *            * rewrote the real number parser, now you can access
 *              it from external and units like eV or erg have no
 *              special status any more.
 *
 *  - 19/09/94  first time created DLL and got it running
 *  - 30/06/97	adaption for LabView/CVI
 *  - 22/08/06  adaption to C++
 *
 */

#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <locale.h>
#include <ctype.h>

#include "units.h"

#define DEBUG_TRACE 1
#define DEBUG_NOISE 2
//#define DEBUG ( DEBUG_TRACE | DEBUG_NOISE )
#define DEBUG ( DEBUG_TRACE )
#include <mgrDebug.h>

#ifdef DEBUG
# include <stdio.h>
#endif

using namespace mgr;

/*
 * unit conversion tables
 *
 * attention: string entries are sorted in EBICD, for ports to
 *            non-MS-DOS systems, the sequence might have to be changed!
 */

namespace mgr {

struct ml_c_unit {
  const char *id;
  double scale;
};

};

static const struct ml_c_unit ml_scale[] = {
  {"%", 1e-2  },
  {"D", 1e1   },
  {"E", 1e18  },
  {"G", 1e9   },
  {"M", 1e6   },
  {"P", 1e15  },
  {"T", 1e12  },
  {"a", 1e-18 },
  {"c", 1e-2  },
  {"d", 1e-1  },
  {"f", 1e-15 },
  {"h", 1e2   },
  {"k", 1e3   },
  {"m", 1e-3  },
  {"n", 1e-9  },
  {"p", 1e-12 },
  // \mu kills gdb!!!
  {"u", 1e-6  },
  {NULL,0     }
};


void FloatFormatter::initLocale() {
  struct lconv *lc = localeconv();
  point = 0;
  group = 0;
  if(lc){
    if(lc->decimal_point) point = *(lc->decimal_point);
    if(lc->thousands_sep) point = *(lc->thousands_sep);
  }
  if(!point) point = '.';
  if(!group) group = ',';  
}

/****** units/ml_cut_value *******************************
*
* NAME
*          ml_cut_value -- read a real number from string
*
* SYNOPSIS
*      next = ml_cut_value(s, value)
*      char *ml_cut_value(char *, double *);
*
* FUNCTION
*      expects the string to begin with a real number and
*      parses its template:
*          SPACE: [ \t\n\r]
*          DIGIT: [0123456789]
*          DOT:   [\.,]
*          SIGN:  [+-]
*          NUMBER:SPACE* {SIGN} DIGIT* {DOT DIGIT*}\
*                 {SPACE* [eE] SPACE* {SIGN} DIGIT*} SPACE*
*      Note that "" is a valid number! The number is then
*      passed to atof() after optional colons have been
*      converted to dots. The result is stored in (value).
*      (next) points to the first character after the
*      template. If in doubt, check for errno to find out,
*      if the template contained a valid number.
*
* INPUTS
*      s       - string to parse
*      value   - result storage
*
* RESULTS
*      next    - next character to deal with
*
* NOTES
*		I never tried to call this function from Visual Basic.
*		Usually the return type (char *) causes big problems
*		with VB, so you might have to change it to (int), if
*		you feel to use this function.
*
* BUGS
*
* SEE ALSO
*      ml_cut_unit()
*
**********************************************************
*
*/

const char *FloatFormatter::read(const char *s, double& v, m_error_t *err) const {
  // skip leading space
  while(isspace(*s)) s++;
  char sign = '+';
  if((*s == '+') || (*s == '-')){
    sign = *s++;
  }
  v = 0.0;
  do{
    while(isdigit(*s)){
      v *= 10.0;
      v += *s - '0';
      s++;
    }
    if(*s == group) s++;
  }while(isdigit(*s));
  if(*s == point){
    double ff = 0.1;
    s++;
    while(isdigit(*s)){
      v += (double)(*s - '0') * ff;
      ff /= 10.0;
      s++;
    }
  }
  if(sign == '-') v = -v;
  const char *t = s;
  // we allow space in between mantissa and exponent
  while(isspace(*s)) s++;
  if(toupper(*s) != 'E') return t;

  // there is an exponent, parse it ...
  s++;
  while(isspace(*s)) s++;
  sign = '+';
  if((*s == '+') || (*s == '-')) sign = *(s++);
  if(!isdigit(*s)){
    if(err) *err = ERR_PARS_STX;
    return t;
  }
  double exponent = 0.0;
  while(isdigit(*s)){
    exponent *= 10.0;
    exponent += *s - '0';
  }
  if(sign == '-') exponent = -exponent;
  v = pow(v,exponent);
  if(err) *err = ERR_NO_ERROR;
  
  return(++s);
}


/****** units/ml_trim *************************************
*
*   NAME
*        ml_trim -- trims a double number to correct scientific notation
*
*   SYNOPSIS
*        error = ml_trim( value,string,precision )
*
*        int ml_trim( double,char *,int );
*
*   FUNCTION
*        trims a double number into correct scientific notation; meaning
*        a mantissa holding not too many zeroes and an exponent which is
*        a multiple of three. It uses precision as the nominal length of
*        the string and seeks the best fitting notation.
*        If the precision is negative, the absolute value is taken and
*        superfluous trailing zeroes are suppressed.
*
*   INPUTS
*        value       - double value to process
*        string      - buffer to put the output string to
*        precision   - number of significant cyphers
*
*   RESULT
*        error       - an error code as defined in <units.h>
*
*   EXAMPLE
*
*   NOTES
*       This function is the verbal copy of mi_Trim() in
*       mathint.c of the MGR, SCalc / DataProc project
*       and thus holds my (Lars Hanke) copyright and not
*       Uni-DO!
*
*   BUGS
*
*   SEE ALSO
*
***********************************************************
*
*   HISTORY
*       20/09/94  * added to MISC.DLL project and wrote
*                   sprintf() stub code
*       22/09/94  * changed return type to int
*/


std::string FloatFormatter::ftoa(double d, int lead, bool cutZeroes) const {
  int space = (lead >= 0)? lead + 1 : 1;
  if(lead > 2 && groupOutput){
    space += lead / 3;
  }
  if(precision > lead){
    space += precision - lead + 1;
  }
  if(d<0) ++space;
  std::string s;
  s.reserve(++space);
  if(d<0){
    s="-"; 
    d = -d;
  } else s="";
  if(lead >= 0){
    double sd = pow(10.0,(double)(-lead));
    sd *= d;
    for(int j=0;j<=lead;++j){
      char v = (char)floor(sd);
      if(v>9){
	pdbg("### Dying in ftoa() line %d (j=%d): v=%d > 9\n",
	     __LINE__,j,(int)v);
	mgrThrowFormat (ERR_INT_STATE,"(j=%d): v=%d > 9 (d: %lg, lead: %d)",j,(int)v,d,lead);
      }
      sd -= v;
      v += '0';
      s.push_back(v);
      if(j && groupOutput && !(j % 3)) s.push_back(group);
      sd *= 10.0;
    }
  } else {
    s.push_back('0');
  }
  if(precision > lead + 1){
    // remove leading fraction
    if(lead >= 0) d -= floor(d);
    if(cutZeroes && (d <= zero)) return s;
    s.push_back('.');
    xpdbg(TRACE,"### post dot mantissa: %lf\n",d);
    if(lead < -1){
      for(int j=-1;j>lead;--j) s.push_back('0');
      d /= pow(10.0,(double)lead);
    } else d *= 10.0;
    int toWrite = (lead >= 0)? precision - lead - 1 : precision;
    for(;toWrite;--toWrite){
      char v = (char)floor(d);
      xpdbg(NOISE,"### toWrite: %d, mantissa: %lf, character: %d, rest: %lg\n",
	    toWrite, d, (int)v, d - (double)v - 1.0);
      if(v>9){
	pdbg("### Dying in ftoa() line %d: v=%d > 9\n",__LINE__,(int)v);
	mgrThrowFormat (ERR_INT_STATE,"(toWrite=%d): v=%d > 9",toWrite,(int)v);
      }
      d -= v;
      d *= 10.0;
      v += '0';
      s.push_back(v);
      if(cutZeroes && (d <= zero)) return s;
    }
  }
  return s;
}

static void putExponent(std::string& s, int e){
  if(e){
    s.reserve(s.length()+7);
    s.push_back('E');
    if(e<0){
      s.push_back('-');
      e = -e;
    }
    int x=10;
    while(x<=e) x *= 10;
    while(e>0){
      x /= 10;
      char v = e / x;
      s.push_back(v + '0');
      e -= v * x;
    }
  }
}

std::string FloatFormatter::trim(double d, bool cutZeroes){

  // insignificantly small
  if(fabs(d) <= zero){
    return "0";
  }

  // cut insignificant digits to 0
  double a=fabs(d);
  int b=(int)floor(log10(a));
  b-=precision;                     /* exponent of cypher to round */
  b++;                              /* put it behind decimal point */
  a*=pow(10.0,(double)(-b));
  a+=.5;
  a=floor(a);

  // determine proper exponent
  int m=(int)floor(log10(a));
  m+=b;                             /* get correct max exp after rounding */
  if(d<0) a=-a;                     /* set correct sign */
  xpdbg(TRACE,"### Mantissa: %.0lf leading %d\n",a,m);
  if(((m>=0) && (m<precision+3)) || /* do not generate mixed notation */
     ((m<0) && (m>-3))){
    a*=pow(10.0,(double)b);         /* we scaled by 10^-b for truncation */
    lastExp = 0;
    return ftoa(a,m,cutZeroes);
  }

  // okay, we use exponential format
  if(m<0) m-=2;
  int e=m / 3;                  /* the exponent to place finally */
  e*=3;
  b-=e;                         /* a*10^b=d=s*10^e ! */
  a*=pow(10.0,(double)b);
  std::string s;
  s = ftoa(a,(int)floor(log10(a)),cutZeroes);
  lastExp = e;
  putExponent(s,e);
  return s;
}

std::string FloatFormatter::trimFix(double d, bool cutZeroes, int e) const {
  double scale = pow(10.0,(double)e);
  d /= scale;
  bool sign = (d<0)? true : false;
  double a = (sign)? -d : d;
  int b = (int)floor(log10(a));
  ++b;
  int xprec = precision;
  if(b<0) xprec += b;
  if(xprec <= 0) return "0";
  a *= pow(10.0,(double)(xprec-b));
  a = floor(a + .5);
  xpdbg(TRACE,"### trimFix(): Cut %lf at %d leading %d\n",a,xprec,b);
  a *= pow(10.0,(double)(b-xprec));
  if(sign) a=-a;

  std::string s;
  s = ftoa(a,b-1+(precision-xprec),cutZeroes);
  putExponent(s,e);

  return s;
}

/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <stdio.h>

struct sFex {
  double val;
  int prec;
  bool cut;
  const char *good;
};

struct sFex format_tests[] = {
  { 1.0,  4, true,  "1" },
  { 1.0,  4, false, "1.000" },
  { -1.0, 4, false, "-1.000" },
  { -0.1, 4, false, "-0.1000" },
  { 12345, 4, true, "12350" },
  { 1234567, 3, false, "1.23E6" },
  { 1234e-7, 4, false, "123.4E-6" },
  { 12345e-7, 4, false, "1.235E-3" },
  { 107.0, 3, false, "107" },
  { 0.0, 4, false, "0" },
  { 0.0, 0, false, NULL }
};

struct sFex fix3_tests[] = {
  { 107.0, 3, false, "0.107E3" },
  { 0.0, 4, false, "0" },
  { 0.0, 0, false, NULL }
};


int main(int argc, char *argv[]){
  size_t tests, errors;

  tests = errors = 0;

  try{
    int fail = 0;
    printf("Test %d: FloatFormatter::trim()\n",++tests);
    struct sFex *cf = format_tests;
    FloatFormatter fmt;
    while(cf && cf->good){
      fmt.setPrecision(cf->prec);
      std::string s = fmt.trim(cf->val,cf->cut);
      if(s != cf->good){
	printf("***");
	++fail;
      } else printf("???");
      printf(" %lg: %s -> %s\n",cf->val,cf->good,s.c_str());
      ++cf;
    }
    if(fail){
      ++errors;
      printf("*** Error: FloatFormatter::trim() failed %d times\n",fail);
    } else {
      puts("+++ FloatFormatter::trim() finished OK!");
    }    

    fail = 0;
    printf("Test %d: FloatFormatter::trimFix()\n",++tests);
    cf = fix3_tests;
    while(cf && cf->good){
      fmt.setPrecision(cf->prec);
      std::string s = fmt.trimFix(cf->val,cf->cut,3);
      if(s != cf->good){
	printf("***");
	++fail;
      } else printf("???");
      printf(" %lg: %s -> %s\n",cf->val,cf->good,s.c_str());
      ++cf;
    }
    if(fail){
      ++errors;
      printf("*** Error: FloatFormatter::trimFix() failed %d times\n",fail);
    } else {
      puts("+++ FloatFormatter::trimFix() finished OK!");
    }    

    printf("Test %d: FloatFormatter::trimFix() combined\n",++tests);
    fail = 0;
    fmt.setPrecision(3);
    std::string vs = fmt.trim(0.00151383);
    if(vs != "1.51E-3"){
      ++fail;
      printf("***");
    } else printf("???");
    printf(" lead trim: %s\n",vs.c_str());
    vs = fmt.trimFix(1.46111e-6);
    if(vs != "0.001E-3"){
      ++fail;
      printf("***");
    } else printf("???");
    printf(" fix trim: %s\n",vs.c_str());
    if(fail){
      ++errors;
      printf("*** Error: FloatFormatter::trimFix() failed %d times\n",fail);
    } else {
      puts("+++ FloatFormatter::trimFix() finished OK!");
    }    

    printf("Test %d: Special FloatFormatter::trim()\n",++tests);
    fail = 0;
    FloatFormatter xmt(3);
    double xv = 1.0 / 3.0;
    std::string s = xmt.trim(xv * 100.0);
    if(s != "33.3"){
      printf("***");
      ++fail;
    } else printf("???");
    printf(" %lg: %s -> %s\n",xv,"33.3",s.c_str());
    if(fail){
      ++errors;
      printf("*** Error: FloatFormatter::trim() failed %d times\n",fail);
    } else {
      puts("+++ FloatFormatter::trim() finished OK!");
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



