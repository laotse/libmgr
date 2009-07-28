#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(libmgr, 2.00, lars@lhanke.de)
#AC_CONFIG_SRCDIR([app/ConsoleFormatter.cpp])
AC_CONFIG_HEADER([config/config.h])
AC_CONFIG_AUX_DIR([config])

AC_SUBST(PACKAGE_TARNAME)
AC_SUBST(PACKAGE_VERSION)

# Check for build host
AC_CANONICAL_HOST
case "$host_os" in
  (dos|nt|win)
      DIR_SEPARATOR="\\"
      cmd_rm="del"
      cmd_rmtree="deltree"
      cmd_mkdir="mkdir"
      cmd_ls="dir";;      
  (*) 
      DIR_SEPARATOR="/"
      cmd_rm="rm -f"
      cmd_rmtree="rm -Rf"
      cmd_mkdir="mkdir"
      cmd_ls="ls";;
esac
AC_SUBST(cmd_rm)
AC_SUBST(cmd_rmtree)
AC_SUBST(cmd_mkdir)
AC_SUBST(cmd_ls)
AC_SUBST(DIR_SEPARATOR)
AC_DEFINE_UNQUOTED(DIR_SEPARATOR_CHAR,'$DIR_SEPARATOR',Character separating components in a path)
AC_DEFINE_UNQUOTED(DIR_SEPARATOR_STRING,"$DIR_SEPARATOR",Character separating components in a path)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Check for system type
AC_EXEEXT
AC_SUBST(EXEEXT)
AC_CANONICAL_TARGET
AC_SUBST(target_cpu)

# FIX source directory
TOPDIR_BASE=`pwd`
TOPDIR=${TOPDIR_BASE}$DIR_SEPARATOR
AC_SUBST(TOPDIR)
# FIXME: should be CONFIG_AUX_DIR
CONFDIR=${TOPDIR}config$DIR_SEPARATOR
AC_SUBST(CONFDIR)

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lexpat':
AC_CHECK_LIB([expat], [XML_ParserCreate] )
AC_CHECK_LIB([m], [exp] )

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h locale.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_STRUCT_TM
AC_C_VOLATILE
AC_C_BIGENDIAN
AC_COMPILE_CHECK_SIZEOF(int)
AC_COMPILE_CHECK_SIZEOF(long)
AC_COMPILE_CHECK_SIZEOF(long long)
AC_COMPILE_CHECK_SIZEOF(double)
AC_COMPILE_CHECK_SIZEOF(float)
AC_COMPILE_CHECK_SIZEOF(short)
AC_COMPILE_CHECK_SIZEOF(char)

# Additional compiler and linker options
COPT="-O2 -Wall -Woverloaded-virtual"
AC_SUBST(COPT)

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([bzero dup2 floor gethostbyname localeconv localtime_r memmove memset pow socket sqrt strdup strtol tzset strtold])

# Check for some structures
AC_CHECK_MEMBERS([struct tm.tm_gmtoff, struct tm.__tm_gmtoff],,,[ 
#include <sys/types.h> 
#include <time.h>] )

# helpers
AC_PATH_PROG(PERL, perl, [])
AC_SUBST(PERL)

AC_CONFIG_FILES([Makefile
                 app/Makefile
                 math/Makefile
                 net/Makefile
                 sol/Makefile
                 templates/Makefile
                 tlv/Makefile
                 util/Makefile
                 xdr/Makefile
                 xml/Makefile
		 crypto/Makefile
		 config/Makefile.machine])
AC_OUTPUT
