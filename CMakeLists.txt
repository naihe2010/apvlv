CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

PROJECT (apvlv)

SET (PROJECT_MAJOR      "0")
SET (PROJECT_MINOR      "1")
SET (PROJECT_PATCH      "2")
SET (PACKAGE_VERSION "${PROJECT_MAJOR}.${PROJECT_MINOR}.${PROJECT_PATCH}")

IF (WIN32)
        ADD_DEFINITIONS (-DDOCDIR=".")
ELSE (WIN32)
        SET (SYSCONFDIR 
             "/etc"
             CACHE PATH
             "Directory for system configuration of apvlv [/etc]"
        )
        ADD_DEFINITIONS (-DSYSCONFDIR="${SYSCONFDIR}")

        SET (DOCDIR 
             "${CMAKE_INSTALL_PREFIX}/share/doc/apvlv"
             CACHE PATH
             "Directory for documentations of apvlv [PREFIX/share/doc/apvlv]"
        )
        ADD_DEFINITIONS (-DDOCDIR="${DOCDIR}")

        SET (MANDIR 
             "${CMAKE_INSTALL_PREFIX}/share/man"
             CACHE PATH
             "Directory for man page of apvlv [PREFIX/share/man]"
        )
ENDIF (WIN32)

ADD_DEFINITIONS (-DPACKAGE_NAME="apvlv"
                 -DPACKAGE_VERSION="${PACKAGE_VERSION}"
                 -DPACKAGE_BUGREPORT="Alf <naihe2010@126.com>"
                 -DRELEASE="rel"
)

FIND_PACKAGE (PkgConfig)
PKG_CHECK_MODULES (FREETYPE2 freetype2 REQUIRED)
PKG_CHECK_MODULES (GTK2 gtk+-2.0 REQUIRED)
PKG_CHECK_MODULES (GLIB2 glib-2.0 REQUIRED)
PKG_CHECK_MODULES (GTHREAD2 gthread-2.0 REQUIRED)
INCLUDE_DIRECTORIES (${FREETYPE2_INCLUDE_DIRS} 
                     ${GTK2_INCLUDE_DIRS} 
                     ${GLIB2_INCLUDE_DIRS} 
                     ${GTHREAD2_INCLUDE_DIRS})
LINK_DIRECTORIES (${FREETYPE2_LIBRARY_DIRS}
                  ${GTK2_LIBRARY_DIRS} 
                  ${GLIB2_LIBRARY_DIRS} 
                  ${GTHREAD2_LIBRARY_DIRS})

IF (WIN32)
        SET (POPPLER_DIR ${CMAKE_HOME_DIRECTORY}/win32/poppler-src)

        INCLUDE_DIRECTORIES (${POPPLER_DIR})
        LINK_DIRECTORIES (${CMAKE_BINARY_DIR}/poppler-build)
        SET (APVLV_REQ_LIBRARIES poppler.lib freetype.lib)

        ADD_SUBDIRECTORY (${POPPLER_DIR} ${CMAKE_BINARY_DIR}/poppler-build)
ELSE (WIN32)
        PKG_CHECK_MODULES (POPPLER poppler-glib REQUIRED)
        INCLUDE_DIRECTORIES (${POPPLER_INCLUDE_DIRS})
        LINK_DIRECTORIES (${POPPLER_LIBRARY_DIRS})
        SET (APVLV_REQ_LIBRARIES ${POPPLER_LIBRARIES})
ENDIF (WIN32)

OPTION (APVLV_WITH_DJVU "If build apvlv with djvu format support." ON)
IF (APVLV_WITH_DJVU)
        ADD_DEFINITIONS (-DHAVE_LIBDJVU)

        IF (WIN32)
                SET (DJVULIBRE_DIR ${CMAKE_HOME_DIRECTORY}/win32/djvulibre)

                INCLUDE_DIRECTORIES (${DJVULIBRE_DIR}/include)
                LINK_DIRECTORIES (${DJVULIBRE_DIR})
                SET (APVLV_REQ_LIBRARIES libdjvulibre.lib
                     ${APVLV_REQ_LIBRARIES})
        ELSE (WIN32)
                SET (APVLV_REQ_LIBRARIES -ldjvulibre
                     ${APVLV_REQ_LIBRARIES})
        ENDIF (WIN32)
ENDIF (APVLV_WITH_DJVU)

OPTION (APVLV_WITH_UMD "If build binary with umd format support." ON)
IF (APVLV_WITH_UMD)
        ADD_DEFINITIONS (-DHAVE_LIBUMD)

        IF (WIN32)
                SET (LIBUMD_DIR ${CMAKE_HOME_DIRECTORY}/win32/libumd-trunk)
                ADD_SUBDIRECTORY (${LIBUMD_DIR} ${CMAKE_BINARY_DIR}/libumd-build)

                INCLUDE_DIRECTORIES (${LIBUMD_DIR})
                LINK_DIRECTORIES (${CMAKE_BINARY_DIR}/libumd-build)
                SET (APVLV_REQ_LIBRARIES umd.lib zdll.lib
                     ${APVLV_REQ_LIBRARIES})
        ELSE (WIN32)
                SET (APVLV_REQ_LIBRARIES -lumd
                     ${APVLV_REQ_LIBRARIES})
        ENDIF (WIN32)
ENDIF (APVLV_WITH_UMD)

ADD_SUBDIRECTORY (src)

IF (WIN32)
        INSTALL (FILES apvlvrc.example DESTINATION "."
                 RENAME _apvlvrc)
        INSTALL (FILES apvlvrc.example Startup.pdf DESTINATION ".")
        INSTALL (DIRECTORY icons DESTINATION "." 
                 PATTERN ".svn" EXCLUDE)
        INSTALL (DIRECTORY ${CMAKE_HOME_DIRECTORY}/win32/poppler-data DESTINATION "." 
                 PATTERN ".svn" EXCLUDE)
ELSE (WIN32)
        INSTALL (FILES apvlvrc.example DESTINATION ${SYSCONFDIR}
                 RENAME apvlvrc)
        INSTALL (FILES apvlvrc.example Startup.pdf Startup.tex DESTINATION ${DOCDIR})
        INSTALL (DIRECTORY icons DESTINATION ${DOCDIR}
                 PATTERN ".svn" EXCLUDE)
        INSTALL (FILES apvlv.1 DESTINATION ${MANDIR}/man1)
ENDIF (WIN32)

SET (CPACK_PACKAGE_VENDOR "Alf")
SET (CPACK_PACKAGE_DESCRIPTION_SUMMARY "apvlv - Alf's PDF/DJVU/UMD Viewer like Vim")
SET (CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_MAJOR})
SET (CPACK_PACKAGE_VERSION_MINOR ${PROJECT_MINOR})
SET (CPACK_PACKAGE_VERSION_PATCH ${PROJECT_PATCH})

IF (UNIX)
        SET (CPACK_SET_DESTDIR ON)
        SET (CPACK_PACKAGE_CONTACT "Alf <naihe2010@126.com>")
        SET (CPACK_GENERATOR RPM DEB)
        SET (CPACK_RPM_PACKAGE_REQUIRES "gtk2 >= 2.10.4")
        SET (CPACK_DEBIAN_PACKAGE_DEPENDS 
             "libpoppler-glib3 (>= 0.8.7-4), poppler-data (>= 0.2.0-2)")

        SET (CPACK_SOURCE_GENERATOR TGZ)
        SET (CPACK_SOURCE_IGNORE_FILES 
             ${CMAKE_BINARY_DIR} 
             ".git"
             ".gitignore"
             ".svn" 
             "win32"
        )
ELSE (UNIX)
        SET (CPACK_GENERATOR NSIS)
        SET (CPACK_NSIS_CONTACT "Alf <naihe2010@126.com>")
        SET (CPACK_NSIS_EXTRA_INSTALL_COMMANDS "CreateShortCut '\$DESKTOP\\\\apvlv.lnk' '\$INSTDIR\\\\apvlv.exe'")
        SET (CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete '\$DESKTOP\\\\gsftc.lnk'")
ENDIF (UNIX)

INCLUDE(CPack)
