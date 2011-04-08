AC_INIT([Complexity],
    m4_esyscmd([./build-aux/git-version-gen .tarball-version]),
    [bkorb@gnu.org])

AS_BOX([Configuring AC_PACKAGE_TARNAME AC_PACKAGE_VERSION])

dnl this should be AC_REQUIRED by gnulib modules that need it,
dnl but either a couple of modules have forgotten it, or else
dnl AC_REQUIRE is emitting macro expansions out of order
dnl
AC_USE_SYSTEM_EXTENSIONS

dnl we can't use AC_CONFIG_AUX_DIR here, because the heuristics
dnl for finding install-sh in the generated configure script
dnl consider this directory to be a subproject of gnulib proper,
dnl and will only look for install-sh in . and .. :(
dnl AC_CONFIG_AUX_DIR([build-aux])
dnl
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile doc/Makefile lib/Makefile src/Makefile tests/Makefile])

AM_INIT_AUTOMAKE([foreign])
LT_INIT
AC_PROG_CC_C99
CX_EARLY
ag_FIND_LIBOPTS
[test -z "${ao_LIBS}" &&] \
    AC_ERROR([libopts was not found])
AM_PROG_CC_C_O
CX_INIT

AC_OUTPUT
