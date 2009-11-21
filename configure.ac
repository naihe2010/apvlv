dnl
dnl APVLV - Alf's PDF Viewer Like Vim
dnl
 
AC_PREREQ([2.61])
AC_INIT(apvlv, 0.0.8.0, naihe2010@gmail.com)
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
AC_PROG_CC
AC_PROG_INSTALL

AC_CHECK_HEADERS([fcntl.h limits.h locale.h stddef.h stdlib.h string.h sys/time.h unistd.h])

AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

AC_FUNC_ERROR_AT_LINE
AC_FUNC_FSEEKO
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor getcwd getpagesize gettimeofday localtime_r memmove memset munmap pow realpath select setlocale sqrt strcasecmp strchr strcspn strdup strerror strncasecmp strrchr strstr strtol])

if test "$prefix" = "NONE"; then
  prefix="/usr/local"
fi

AC_DEFINE_UNQUOTED(PREFIX, ["$prefix"], [The prefix path])

sysconfdir="/etc"
AC_ARG_WITH(sysconfdir,
            [[  --with-sysconfdir=DIR       directory for system configuration of apvlv [/etc]]], 
            [ case "$withval" in
             yes|no) 
             ;;
             *) sysconfdir="$withval"
             ;;
           esac])
AC_SUBST(SYSCONFDIR)
AC_DEFINE_UNQUOTED(SYSCONFDIR, ["$sysconfdir"], [The system configuration directory])

docdir="$prefix/share/doc/$PACKAGE_TARNAME"
AC_ARG_WITH(docdir,
            [[  --with-docdir=DIR       directory for documentations of apvlv [PREFIX/share/doc/apvlv]]], 
            [ case "$withval" in
             yes|no) 
             ;;
             *) docdir="$withval"
             ;;
           esac])
AC_SUBST(DOCDIR)
AC_DEFINE_UNQUOTED(DOCDIR, ["$docdir"], [The documents directory])

mandir="$prefix/man"
AC_ARG_WITH(mandir,
            [[  --with-mandir=DIR       directory for man page of apvlv [PREFIX/man]]], 
            [ case "$withval" in
             yes|no) 
             ;;
             *) mandir="$withval"
             ;;
           esac])
AC_SUBST(MANDIR)
AC_DEFINE_UNQUOTED(MANDIR, ["$mandir"], [The man page directory])

if test x$apvlv_debug = xyes; then 
  AC_DEFINE(DEBUG, 1, 
            [Define if you want to enable apvlv debug features. ]) 
fi

AC_CHECK_HEADER([libdjvu/ddjvuapi.h],[
                 AC_CHECK_LIB(djvulibre, [ddjvu_document_create_by_filename],
                              AC_DEFINE([HAVE_LIBDJVU], 1, [Define to 1 if you have libdjvu]) 
                              LIBS="-ldjvulibre $LIBS"
                              , AC_MSG_WARN(Libdjvu library was not found))
                 ], )

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.4.0, CPPFLAGS="$CPPFLAGS $GTK_CFLAGS"; LIBS="$LIBS $GTK_LIBS")
PKG_CHECK_MODULES(POPPLER, poppler-glib >= 0.5.4, CPPFLAGS="$CPPFLAGS $POPPLER_CFLAGS"; LIBS="$LIBS $POPPLER_LIBS")

AC_OUTPUT(Makefile src/Makefile)
