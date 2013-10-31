AC_INIT([Complexity],
    m4_esyscmd([cat .tarball-version]),
    [bkorb@gnu.org])

AS_BOX([Configuring AC_PACKAGE_TARNAME AC_PACKAGE_VERSION])

dnl we can't use AC_CONFIG_AUX_DIR here, because the heuristics
dnl for finding install-sh in the generated configure script
dnl consider this directory to be a subproject of gnulib proper,
dnl and will only look for install-sh in . and .. :(
dnl AC_CONFIG_AUX_DIR([build-aux])
dnl
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile doc/Makefile lib/Makefile src/Makefile tests/Makefile])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/complexity.c])
AM_INIT_AUTOMAKE([gnu check-news 1.5 dist-xz])
AC_USE_SYSTEM_EXTENSIONS
gl_INIT

AC_PROG_CC_C99

gl_EARLY
ag_FIND_LIBOPTS
[test -z "${ao_LIBS}" && \
    ]AC_ERROR([libopts was not found])[

if autogen -MF/tmp/ag-bogus-$$ --help >/dev/null
then ag_cv_ag_supports_mf=true
else ag_cv_ag_supports_mf=false
fi
]
AM_CONDITIONAL([AG_MF],[$ag_cv_ag_supports_mf])
AM_PROG_CC_C_O

AC_OUTPUT
