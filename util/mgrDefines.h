/*
 * Some common definitions about debugging, error reporting
 * and some common shorthands
 *
 * Include this file after all standard includes, i.e. after the
 * #include <include_file.h> section, but before the lib includes, i.e. before
 * #include "../lib/some_file.h"
 *
 * (c) 2004 T-Systems GEI GmbH
 * Author:   Dr. Lars Hanke
 * Revised:  05.01.2004
 * Revision: 1.0
 *
 */

#ifndef _LIB_MGR_DEFINES_H_
#define _LIB_MGR_DEFINES_H_

// Print to stderr
#define perr(args...) fprintf(stderr, ## args)

// Print user info
#define pusr(args...) fprintf(USER_INFO, ## args)

// Print debug messages to output stream
// moved to mgrDebug.h

// Some handy defines
#define alloc_struct(x) (struct x *)malloc(sizeof(struct x))
#define alloc_array(n,x) (x *)malloc((n)*sizeof(x))
#define alloc_nstruct(n,x) alloc_array((n),struct x)

#endif // _LIB_MGR_DEFINES_H_
