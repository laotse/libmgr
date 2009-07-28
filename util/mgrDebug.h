/*
 *
 * Debugging support functions
 *
 * (c) 2005 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: mgrDebug.h,v 1.4 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  pdbg  - a printf variant dumping to DEBUG_LOG and turns to naught, 
 *          if DEBUG is not set
 *  xpdbg - as pdbg, but depending on the result of DEBUG_CHECK(arg1)
 *
 * This defines the values:
 *
 * This defines the macro system:
 *
 *  define DEBUG_[ASPECT] macros as DEBUG_ASPECT (1 << #)
 *  define DEBUG as logical or of aspects to debug, e.g.
 *  define DEBUG (DEBUG_AS1 | DEBUG_AS2)
 *
 *  You may then use constructs like:
 *
 *  #if DEBUG_CHECK(AS1)         - true if AS1 is set in DEBUG
 *  #if DEBUG_REQUIRE(DEBUG_AS1) - true if the entire mask is set in DEBUG
 *  #if DEBUG_MASK(DEBUG_AS1)    - true if any parts of the mask are set 
 *                                 in DEBUG
 *  in case DEBUG is not defined, all these macros expand to 0
 *  try #define DEBUG_COMBI (DEBUG_AS1 | DEBUG_AS2) for handy combinations
 *  and
 *  xpdbg(AS1,"Debug aspect 1 yields %d\n",some_var);
 *    will effectively check DEBUG_REQUIRE() on COMBI!
 *
 */


//! Disable all debugging by using -DNO_DEBUG during compile
#ifdef NO_DEBUG
# undef DEBUG
#endif

#ifndef _UTIL_DEBUG_H_
//! This means we parsed this file
# define _UTIL_DEBUG_H_
#endif

#ifdef DEBUG
# include <stdio.h>

// Print debug messages to output stream
# ifndef DEBUG_LOG
//! Stream to write debugging information - default stderr
#  define DEBUG_LOG stderr
# endif
//! Check if the flag DEBUG_x is set in DEBUG
/*! \param x Flag name without leading DEBUG_ */
# define DEBUG_CHECK(x) ((DEBUG_ ## x) && ((DEBUG & (DEBUG_ ## x)) == (DEBUG_ ## x)))
//! Check if the mask x is set in DEBUG
/*! \param x Several DEBUG_# flags ored together */
# define DEBUG_REQUIRE(x) ((DEBUG & (x)) == (x))
//! Check if any of the mask x is set in DEBUG
/*! \param x Several DEBUG_# flags ored together */
# define DEBUG_MASK(x) (DEBUG & (x))
//! Unconditionally print debugging information
/*! Prints debugging information to DEBUG_LOG, if DEBUG
    is set and not diabled by NO_DEBUG. The function
    itself is identical to printf(). Printing does
    not depend on which flags are set in the DEBUG
    define.
*/
# define pdbg(...) fprintf((DEBUG_LOG), __VA_ARGS__ )
//! Conditionally print debugging information
/*! Prints debugging information to DEBUG_LOG, if DEBUG
    contains the DEBUG_x flag and not diabled by NO_DEBUG. 
    The function itself is identical to printf(). 
*/
# define xpdbg(x,...) do{ if(DEBUG_CHECK(x)) pdbg(__VA_ARGS__ ); } while(0)
#else // DEBUG
# define pdbg(...) while(0)
# define xpdbg(...) while(0)
# define DEBUG_CHECK(x) 0
# define DEBUG_REQUIRE(x) 0
# define DEBUG_MASK(x) 0
#endif //DEBUG
