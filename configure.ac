dnl
dnl APVLV - Alf's PDF Viewer Like Vim
dnl
 
AC_INIT(apvlv, 0.0.4.5, naihe2010@gmail.com)
AC_COPYRIGHT([Copyright (C) 2008 Alf's Software.])

if test -e .svn; then
  RELEASE=`svn info | grep 'Last Changed Rev:' | awk '{print $4}'`
else
  RELEASE='nosvn'
fi

AC_SUBST(RELEASE)
AC_DEFINE_UNQUOTED(RELEASE,["$RELEASE"],[The release number of package])

AC_ARG_ENABLE(debug,
              [  --enable-debug          enable apvlv debug features],
              [apvlv_debug=yes])

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

AC_DEFINE_UNQUOTED(PREFIX, ["$prefix"], [The prefix path])

if test x$apvlv_debug = xyes; then 
  AC_DEFINE(DEBUG, 1, 
            [Define if you want to enable apvlv debug features. ]) 
fi

AC_CHECK_HEADER([pthread.h],[
                 AC_CHECK_LIB(pthread, [pthread_create],
                              AC_DEFINE([HAVE_PTHREAD], 1, [Define to 1 if you have pthread]) 
                              LIBS="-lpthread $LIBS"
                              , AC_MSG_WARN(pthread library was not found))
                 ], )

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.4.0, CPPFLAGS="$CPPFLAGS $GTK_CFLAGS"; LIBS="$LIBS $GTK_LIBS")
PKG_CHECK_MODULES(POPPLER, poppler-glib >= 0.5.4, CPPFLAGS="$CPPFLAGS $POPPLER_CFLAGS"; LIBS="$LIBS $POPPLER_LIBS")

dnl
dnl check whether Startup.pdf should be created by self.
dnl
AC_PATH_PROG(latexpath, latex)
AC_PATH_PROG(dvipdfpath, dvipdf)

AC_OUTPUT(Makefile src/Makefile)
