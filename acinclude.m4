dnl acinclude.m4 -- This file is part of VMIPS.
dnl $Id: acinclude.m4,v 1.4 2000/05/09 08:14:33 brg Exp $
dnl It is used to build the configure script. See configure.in for details.

dnl Local macro: VMIPS_STATIC_GETPWNAM
dnl Can libtool compile statically linked programs that call getpwnam()?
dnl On Solaris the answer is no, and we must dynamically link with libdl.
dnl Based on AC_TRY_LINK from acgeneral.m4.

AC_DEFUN(VMIPS_STATIC_GETPWNAM,
[AC_CACHE_CHECK([whether programs calling getpwnam can be statically linked],
[vmips_cv_static_getpwnam],[cat > conftest.$ac_ext <<EOF
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
vmips_cv_static_getpwnam=no
if AC_TRY_COMMAND(./libtool --mode=compile $CXX $CXXFLAGS -c -o conftest.o conftest.$ac_ext 2>&AC_FD_CC 1>&AC_FD_CC)
then
	if AC_TRY_COMMAND(./libtool --mode=link $CXX $CXXFLAGS -all-static -o conftest${ac_exeext} conftest.o 2>&AC_FD_CC 1>&AC_FD_CC)
	then
		vmips_cv_static_getpwnam=yes
	fi
fi
if test "x$vmips_cv_static_getpwnam" = "xyes"
then
  true
else
  echo "configure: failed program was:" >&AC_FD_CC
  cat conftest.$ac_ext >&AC_FD_CC
fi
rm -rf conftest*])
if test "x$vmips_cv_static_getpwnam" = "xyes"
then
  SOLARIS_DL_HACK=""
else
  SOLARIS_DL_HACK="/usr/lib/libdl.so"
fi
AC_SUBST(SOLARIS_DL_HACK)])

