dnl
dnl APVLV - Alf's PDF Viewer Like Vim
dnl
 
AC_INIT(apvlv, 0.0.1, naihe2010@gmail.com)
AC_COPYRIGHT([Copyright (C) 2008 Alf's Software.])

if test -e .svn; then
  RELEASE=`svn info | grep 'Last Changed Rev:' | awk '{print $4}'`
else
  RELEASE='nosvn'
fi

AC_SUBST(RELEASE)
AC_DEFINE_UNQUOTED(RELEASE,["$RELEASE"],[The release number of package])

AM_CONFIG_HEADER(src/config.hpp)
AC_CONFIG_AUX_DIR(scripts)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE

AC_PROG_CXX
AC_PROG_LD
AC_PROG_INSTALL

if test "$prefix" = "NONE"; then
   prefix="/usr/local"
fi

GTK2_REQUIRED=2.4.0
PKG_CHECK_MODULES(GTK, gtk+-2.0, CPPFLAGS="$CPPFLAGS $GTK_CFLAGS"; LIBS="$LIBS $GTK_LIBS")
PKG_CHECK_MODULES(POPPLER, poppler-glib, CPPFLAGS="$CPPFLAGS $POPPLER_CFLAGS"; LIBS="$LIBS $POPPLER_LIBS")

AC_OUTPUT(Makefile src/Makefile)
