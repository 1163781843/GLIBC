#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([glibc_malloc_tcache.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

AM_INIT_AUTOMAKE
LT_INIT

dnl GLIB_LDFLAGS="${LD_FLAGS}"
dnl AC_SUBST([GLIB_LDFLAGS])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([malloc.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memset])

AC_CONFIG_FILES([
    Makefile
])

AC_OUTPUT
