dnl aclocal.m4 generated automatically by aclocal 1.4-p4

dnl Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.

dnl acinclude.m4 -- This file is part of VMIPS.
dnl It is used to build the configure script. See configure.in for details.

dnl Local macro: VMIPS_CXX_ATTRIBUTE_NORETURN
dnl Check for availability of __attribute__((noreturn)) syntax for specifying
dnl that a function will never return. Defines HAVE_ATTRIBUTE_NORETURN if it
dnl works. We assume that if the attribute-adorned function compiles without
dnl giving a warning, then it is supported.

AC_DEFUN(VMIPS_CXX_ATTRIBUTE_NORETURN,
[AC_CACHE_CHECK([whether specifying that a function will never return works],
[vmips_cv_cxx_attribute_noreturn],
[if test "x$GXX" = "xyes" 
 then
   (CXXFLAGS="-Werror $CXXFLAGS"
    AC_LANG_CPLUSPLUS
    AC_TRY_COMPILE([#include <cstdlib>
      __attribute__((noreturn)) void die(void) { abort(); }],
      [],exit 0,exit 1))
   if test $? -eq 0
   then
     vmips_cv_cxx_attribute_noreturn=yes
   else
     vmips_cv_cxx_attribute_noreturn=no
   fi
 else
   vmips_cv_cxx_attribute_noreturn=no
 fi])
if test "$vmips_cv_cxx_attribute_noreturn" = yes
then
  AC_DEFINE(HAVE_ATTRIBUTE_NORETURN, 1,
  [Define if __attribute__((noreturn)) syntax can be used to specify
   that a function will never return.])
fi])

dnl Local macro: VMIPS_CXX_ATTRIBUTE_FORMAT
dnl Check for availability of __attribute__((format (...))) syntax for
dnl specifying that a function takes printf-style arguments. Defines
dnl HAVE_ATTRIBUTE_PRINTF if it works. As with VMIPS_CXX_ATTRIBUTE_NORETURN,
dnl we assume that if the attribute-adorned function compiles without giving
dnl a warning, then it is supported.

AC_DEFUN(VMIPS_CXX_ATTRIBUTE_FORMAT,
[AC_CACHE_CHECK([whether specifying that a function takes printf-style arguments works], [vmips_cv_cxx_attribute_format],
[if test "x$GXX" = "xyes"
then
  (CXXFLAGS="-Werror $CXXFLAGS"
   AC_LANG_CPLUSPLUS
   AC_TRY_COMPILE([#include <cstdlib>
     __attribute__((format(printf, 1, 2)))
     void myprintf(char *fmt, ...) { abort(); }],[],exit 0,exit 1))
  if test $? -eq 0
  then
    vmips_cv_cxx_attribute_format=yes
  else
    vmips_cv_cxx_attribute_format=no
  fi
else
  vmips_cv_cxx_attribute_format=no
fi])
if test "x$vmips_cv_cxx_attribute_format" = "xyes"
then
  AC_DEFINE(HAVE_ATTRIBUTE_FORMAT, 1,
  [True if __attribute__((format (...))) syntax can be used to specify
   that a function takes printf-style arguments.])
fi])

dnl Local macro: VMIPS_CXX_TEMPLATE_FUNCTIONS
dnl Check for template function handling bug in, for example, pre-2.95 g++.
dnl Abort with a configuration-time error if the test program doesn't compile.

AC_DEFUN(VMIPS_CXX_TEMPLATE_FUNCTIONS,
[AC_CACHE_CHECK([whether you can pass a template function to a function whose return type is the same as the type of its parameter],
[vmips_cv_cxx_template_functions],
[(AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([
    template<class F> F x(F f) { }
    template<class T> void y(T t) { }
    void z(void) { x(y<int>); }
  ],[],exit 0,exit 1))
if test $? -eq 0
then
  vmips_cv_cxx_template_functions=yes
else
  vmips_cv_cxx_template_functions=no
fi])
if test "x$vmips_cv_cxx_template_functions" = "xno"
then
  AC_MSG_ERROR([your C++ compiler's template function handling is buggy; see INSTALL])
fi])

dnl Local macro: VMIPS_TYPE_SOCKLEN_T
dnl #define socklen_t to int if socklen_t is not in sys/socket.h.

AC_DEFUN(VMIPS_TYPE_SOCKLEN_T,
[AC_CACHE_CHECK([for socklen_t], [vmips_cv_type_socklen_t],
[AC_TRY_COMPILE(
[#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>],
[socklen_t foo; foo = 1; return 0;],
vmips_cv_type_socklen_t=yes, vmips_cv_type_socklen_t=no)])
if test "x$vmips_cv_type_socklen_t" = "xno"
then
  AC_DEFINE(socklen_t, int,
    [Define to the type pointed to by the third argument to getsockname,
     if it is not socklen_t.])
fi])

dnl Local macro: VMIPS_LINK_STATIC_GETPWNAM
dnl Can libtool compile statically linked programs that call getpwnam()?
dnl On Solaris the answer is no, and we must dynamically link with libdl.
dnl Based on AC_TRY_LINK from acgeneral.m4.

AC_DEFUN(VMIPS_LINK_STATIC_GETPWNAM,
[AC_CACHE_CHECK([whether programs calling getpwnam can be statically linked],
[vmips_cv_link_static_getpwnam],[cat > conftest.$ac_ext <<EOF
[#]line __oline__ "configure"
#include "confdefs.h"
#include <stdio.h>
#include <pwd.h>
int main() {
struct passwd *p;
p = getpwnam("root");
printf("%s\n", p->pw_name);
return 0; }
EOF
vmips_cv_link_static_getpwnam=no
if AC_TRY_COMMAND(./libtool --mode=compile $CXX $CXXFLAGS -c -o conftest.o conftest.$ac_ext 2>&AC_FD_CC 1>&AC_FD_CC)
then
  if AC_TRY_COMMAND(./libtool --mode=link $CXX $CXXFLAGS -all-static -o conftest${ac_exeext} conftest.o 2>&AC_FD_CC 1>&AC_FD_CC)
  then
    vmips_cv_link_static_getpwnam=yes
  fi
fi
if test "x$vmips_cv_link_static_getpwnam" = "xyes"
then
  true
else
  echo "configure: failed program was:" >&AC_FD_CC
  cat conftest.$ac_ext >&AC_FD_CC
fi
rm -rf conftest*])
if test "x$vmips_cv_link_static_getpwnam" = "xyes"
then
  SOLARIS_DL_HACK=""
else
  SOLARIS_DL_HACK="/usr/lib/libdl.so"
fi
AC_SUBST(SOLARIS_DL_HACK)])


# Do all the work for Automake.  This macro actually does too much --
# some checks are only needed if your package does certain things.
# But this isn't really a big deal.

# serial 1

dnl Usage:
dnl AM_INIT_AUTOMAKE(package,version, [no-define])

AC_DEFUN(AM_INIT_AUTOMAKE,
[AC_REQUIRE([AC_PROG_INSTALL])
PACKAGE=[$1]
AC_SUBST(PACKAGE)
VERSION=[$2]
AC_SUBST(VERSION)
dnl test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" && test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi
ifelse([$3],,
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package]))
AC_REQUIRE([AM_SANITY_CHECK])
AC_REQUIRE([AC_ARG_PROGRAM])
dnl FIXME This is truly gross.
missing_dir=`cd $ac_aux_dir && pwd`
AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
AM_MISSING_PROG(MAKEINFO, makeinfo, $missing_dir)
AC_REQUIRE([AC_PROG_MAKE_SET])])

#
# Check to make sure that the build environment is sane.
#

AC_DEFUN(AM_SANITY_CHECK,
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftestfile
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftestfile 2> /dev/null`
   if test "[$]*" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftestfile`
   fi
   if test "[$]*" != "X $srcdir/configure conftestfile" \
      && test "[$]*" != "X conftestfile $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "[$]2" = conftestfile
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
rm -f conftest*
AC_MSG_RESULT(yes)])

dnl AM_MISSING_PROG(NAME, PROGRAM, DIRECTORY)
dnl The program must properly implement --version.
AC_DEFUN(AM_MISSING_PROG,
[AC_MSG_CHECKING(for working $2)
# Run test in a subshell; some versions of sh will print an error if
# an executable is not found, even if stderr is redirected.
# Redirect stdin to placate older versions of autoconf.  Sigh.
if ($2 --version) < /dev/null > /dev/null 2>&1; then
   $1=$2
   AC_MSG_RESULT(found)
else
   $1="$3/missing $2"
   AC_MSG_RESULT(missing)
fi
AC_SUBST($1)])

# Like AC_CONFIG_HEADER, but automatically create stamp file.

AC_DEFUN(AM_CONFIG_HEADER,
[AC_PREREQ([2.12])
AC_CONFIG_HEADER([$1])
dnl When config.status generates a header, we must update the stamp-h file.
dnl This file resides in the same directory as the config header
dnl that is generated.  We must strip everything past the first ":",
dnl and everything past the last "/".
AC_OUTPUT_COMMANDS(changequote(<<,>>)dnl
ifelse(patsubst(<<$1>>, <<[^ ]>>, <<>>), <<>>,
<<test -z "<<$>>CONFIG_HEADERS" || echo timestamp > patsubst(<<$1>>, <<^\([^:]*/\)?.*>>, <<\1>>)stamp-h<<>>dnl>>,
<<am_indx=1
for am_file in <<$1>>; do
  case " <<$>>CONFIG_HEADERS " in
  *" <<$>>am_file "*<<)>>
    echo timestamp > `echo <<$>>am_file | sed -e 's%:.*%%' -e 's%[^/]*$%%'`stamp-h$am_indx
    ;;
  esac
  am_indx=`expr "<<$>>am_indx" + 1`
done<<>>dnl>>)
changequote([,]))])

#serial 1
# This test replaces the one in autoconf.
# Currently this macro should have the same name as the autoconf macro
# because gettext's gettext.m4 (distributed in the automake package)
# still uses it.  Otherwise, the use in gettext.m4 makes autoheader
# give these diagnostics:
#   configure.in:556: AC_TRY_COMPILE was called before AC_ISC_POSIX
#   configure.in:556: AC_TRY_RUN was called before AC_ISC_POSIX

undefine([AC_ISC_POSIX])

AC_DEFUN([AC_ISC_POSIX],
  [
    dnl This test replaces the obsolescent AC_ISC_POSIX kludge.
    AC_CHECK_LIB(cposix, strerror, [LIBS="$LIBS -lcposix"])
  ]
)

