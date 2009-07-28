/*********************************************************
 *
 * C - Source file identification Header
 *
 * Copyright (c) 1994 by MGR - Software, Asgard
 * written 1994 by Lars Hanke
 *
 *********************************************************
 *
 * PROJECT: Wavemeter
 *  MODULE: misc.lib
 *    FILE: units.h
 *
 *********************************************************
 *
 *   AUTHOR: Lars Hanke
 *  REVISOR: Lars Hanke
 * REVISION: 0.9
 *     DATE: 14/09/94
 *
 *********************************************************
 *
 * DESCRIPTION
 *      These are the includes and definitions of units.c
 *
 * EXPORTS
 *
 * IMPORTS
 *
 *********************************************************
 *
 * HISTORY
 *
 */

#ifndef _MATH_UNITS_H
#define _MATH_UNITS_H

#include <mgrError.h>
#include <string>

namespace mgr {

class FloatFormatter {
protected:
  char group;       // thousands grouping character
  char point;       // decimal point
  bool groupOutput; // use group character for output

  int precision;    // number of significant digits
  double rprec;     // a small number
  double zero;      // insignificantly small value

  int lastExp;      // Exponent used with last trim()

  void initLocale();
  std::string ftoa(double d, int lead, bool cutZeroes = false) const;
  inline double floor(double d) const {
    return ::floor(d + rprec);
  }

public:
  FloatFormatter( const int& prec = 3 ) 
    : groupOutput(false), precision(prec), zero(0.0), lastExp(0) 
  { initLocale(); setPrecision((prec<0)? -prec : prec); }

  m_error_t setPrecision(const int& p){
    if(p < 1) return ERR_PARAM_RANG;
    precision = p;
    rprec = pow(10.0,-(precision + 2));
    return ERR_NO_ERROR;
  }

  m_error_t setDelimiters(const char& pDel, const char& gDel){
    if(!pDel){
      initLocale();
      return ERR_NO_ERROR;
    }
    point = pDel;
    group = gDel;
  }

  void setZero(const double& epsilon){
    zero = fabs(epsilon);
  }

  const char *read(const char *s, double &val, m_error_t *err = NULL) const;
  // if non-const pointer is provided, non-const pointer is returned
  char *read(char *s, double &val, m_error_t *err = NULL) const {
    return const_cast<char *>(read(const_cast<const char *>(s),val,err));
  }
  // format a double number
  std::string trim(double d, bool cutZeroes = false);
  std::string trimFix(double d, bool cutZeroes, int e) const;
  std::string trimFix(double d, bool cutZeroes = false) const {
    return trimFix(d,cutZeroes,lastExp);
  }
};

}; // namespace mgr

#endif // _MATH_UNITS_H
