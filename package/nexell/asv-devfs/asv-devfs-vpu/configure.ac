#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.10 foreign dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
# FIXME: Replace `main' with a function in `-ldrm':
AC_CHECK_LIB([drm], [main])
# FIXME: Replace `main' with a function in `-lnx_video_alloc':
AC_CHECK_LIB([nx_video_alloc], [main])
# FIXME: Replace `main' with a function in `-lnx_video_api':
AC_CHECK_LIB([nx_video_api], [main])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset mkdir strdup])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
