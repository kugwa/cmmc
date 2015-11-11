#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# vim: set sw=4 ts=4 sts=4 et:

AC_INIT([ccmmc], [0.1])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/lexer.l])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_RANLIB
AC_PROG_LEX
AC_PROG_YACC

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

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
