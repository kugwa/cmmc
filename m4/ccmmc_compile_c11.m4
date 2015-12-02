# The macro CCMMC_COMPILE_C11 is a simplified version of AX_CXX_COMPILE_STDCXX
# that is modified to work with C11. The version we based on is 'serial 1',
# which can be easily found on GNU Savannah:
#   http://git.savannah.gnu.org/cgit/autoconf-archive.git
#
# The path of the file is m4/ax_cxx_compile_stdcxx.m4.
#
# LICENSE OF AX_CXX_COMPILE_STDCXX
#
#   Copyright (c) 2008 Benjamin Kosnik <bkoz@redhat.com>
#   Copyright (c) 2012 Zack Weinberg <zackw@panix.com>
#   Copyright (c) 2013 Roy Stogner <roystgnr@ices.utexas.edu>
#   Copyright (c) 2014, 2015 Google Inc.; contributed by Alexey Sokolov <sokolov@google.com>
#   Copyright (c) 2015 Paul Norman <penorman@mac.com>
#   Copyright (c) 2015 Moritz Klammler <moritz@klammler.eu>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.  This file is offered as-is, without any
#   warranty.
#
#
# The content of _CCMMC_COMPILE_C11_testbody is based on the development
# git snapshot of GNU autoconf, which can be easily found on GNU Savannah:
#   http://git.savannah.gnu.org/cgit/autoconf.git
#
# The path of the file is lib/autoconf/c.m4.
#
# LICENSE OF GNU AUTOCONF
#
# Copyright (C) 2001-2015 Free Software Foundation, Inc.
#
# This file is part of Autoconf.  This program is free
# software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# Under Section 7 of GPL version 3, you are granted additional
# permissions described in the Autoconf Configure Script Exception,
# version 3.0, as published by the Free Software Foundation.
#
# You should have received a copy of the GNU General Public License
# and a copy of the Autoconf Configure Script Exception along with
# this program; see the files COPYINGv3 and COPYING.EXCEPTION
# respectively.  If not, see <http://www.gnu.org/licenses/>.
#
# Written by David MacKenzie, with help from
# Akim Demaille, Paul Eggert,
# Franc,ois Pinard, Karl Berry, Richard Pixley, Ian Lance Taylor,
# Roland McGrath, Noah Friedman, david d zuhn, and many others.
#
#
# This file is free software licensed under the term that is the
# same as GNU autoconf.
#
# vim: set sw=2 ts=2 sts=2 et:

AC_DEFUN([CCMMC_COMPILE_C11], [
  AC_LANG_PUSH([C])
  ac_success=no
  AC_CACHE_CHECK(
    [whether $CC supports C11 features by default],
    [ccmmc_cv_compile_c11],
    [AC_COMPILE_IFELSE([AC_LANG_SOURCE([_CCMMC_COMPILE_C11_testbody])],
      [ccmmc_cv_compile_c11=yes],
      [ccmmc_cv_compile_c11=no])])
  if test x$ccmmc_cv_compile_c11 = xyes; then
    ac_success=yes
  fi
  if test x$ac_success = xno; then
    for switch in -std=gnu11 -std=c11 -std=gnu1x -std=c1x; do
      cachevar=AS_TR_SH([ccmmc_compile_c11_$switch])
      AC_CACHE_CHECK(
        [whether $CC supports C11 features with $switch],
        [$cachevar],
        [ac_save_CFLAGS="$CFLAGS"
         CFLAGS="$CFLAGS $switch"
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([_CCMMC_COMPILE_C11_testbody])],
           [eval $cachevar=yes],
           [eval $cachevar=no])
         CFLAGS="$ac_save_CFLAGS"])
      if eval test x\$$cachevar = xyes; then
        CFLAGS="$CFLAGS $switch"
        ac_success=yes
        break
      fi
    done
  fi
  AC_LANG_POP([C])
  if test x$ac_success = xno; then
    HAVE_C11=0
  else
    HAVE_C11=1
    AC_DEFINE([HAVE_C11], [1],
      [define if the compiler supports basic C11 syntax])
  fi
  AC_SUBST([HAVE_C11])
])

m4_define([_CCMMC_COMPILE_C11_testbody], [[
// Check whether static_assert macro works
#include <assert.h>
static_assert (1, "It works!");

// Check _Alignas.
char _Alignas (double) aligned_as_double;
char _Alignas (0) no_special_alignment;
extern char aligned_as_int;
char _Alignas (0) _Alignas (int) aligned_as_int;

// Check _Alignof.
enum
{
  int_alignment = _Alignof (int),
  int_array_alignment = _Alignof (int[100]),
  char_alignment = _Alignof (char)
};
_Static_assert (0 < -_Alignof (int), "_Alignof is signed");

// Check _Noreturn.
int _Noreturn does_not_return (void) { for (;;) continue; }

// Check _Static_assert.
struct test_static_assert
{
  int x;
  _Static_assert (sizeof (int) <= sizeof (long int),
                  "_Static_assert does not work in struct");
  long int y;
};

// Check UTF-8 literals.
// Commented out because we don't use them.
// #define u8 syntax error!
// char const utf8_literal[] = u8"happens to be ASCII" "another string";

// Check duplicate typedefs.
typedef long *long_ptr;
typedef long int *long_ptr;
typedef long_ptr long_ptr;

// Anonymous structures and unions -- taken from C11 6.7.2.1 Example 1.
struct anonymous
{
  union {
    struct { int i; int j; };
    struct { int k; long int l; } w;
  };
  int m;
} v1;
]])
