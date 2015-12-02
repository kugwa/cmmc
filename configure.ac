#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# vim: set sw=4 ts=4 sts=4 et:

AC_INIT([ccmmc], [0.1])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/lexer.l])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])

# We need POSIX functions. OS specific notes:
# FreeBSD: Defining these macros limits C standard version to C99.
# NetBSD: libc doesn't define static_assert.
# OpenBSD: libc doesn't define static_assert.
case "$host_os" in
    *freebsd*)
        ;;
    *netbsd*|*openbsd*)
        AC_DEFINE([static_assert], [_Static_assert], [C11 compatiblity])
        ;;
    *)
        AC_DEFINE([_POSIX_C_SOURCE], [200809L], [Enable POSIX.1-2008 support])
        AC_DEFINE([_XOPEN_SOURCE], [700], [Enable X/OPEN system interfaces])
        ;;
esac

# Checks for programs.
AC_PROG_CC
CCMMC_COMPILE_C11
AC_PROG_CC_STDC
AC_PROG_RANLIB
AC_PROG_LEX
AC_PROG_YACC

if test x${HAVE_C11} = x0; then
    AC_MSG_ERROR([C compiler with C11 support is required.])
fi

# Checks for warning flags.
AX_IS_RELEASE([git-directory])
AX_COMPILER_FLAGS()
case "$WARN_CFLAGS" in
    *-Wdeclaration-after-statement*)
        WARN_CFLAGS="$WARN_CFLAGS -Wno-declaration-after-statement"
        ;;
esac

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_STRERROR_R

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
