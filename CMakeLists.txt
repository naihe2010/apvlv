CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

PROJECT (apvlv)

SET (PROJECT_MAJOR      "0")
SET (PROJECT_MINOR      "1")
SET (PROJECT_PATCH      "0")
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
             "${CMAKE_PREFIX}/share/man"
             CACHE PATH
             "Directory for man page of apvlv [PREFIX/share/man]"
        )
ENDIF (WIN32)

ADD_DEFINITIONS (-DPACKAGE_NAME="apvlv"
                 -DPACKAGE_VERSION="${PACKAGE_VERSION}"
                 -DPACKAGE_BUGREPORT="Alf <naihe2010@gmail.com>"
                 -DRELEASE="rel"
)

ADD_SUBDIRECTORY (src)

IF (WIN32)
        FIND_FILE (LIBATK libatk-1.0-0.dll)
        FIND_FILE (LIBEXPAT libexpat-1.dll)
        FIND_FILE (LIBFONTCONFIG libfontconfig-1.dll)
        FIND_FILE (LIBGIO libgio-2.0-0.dll)
        FIND_FILE (LIBCAIRO libcairo-2.dll)
        FIND_FILE (LIBPANGOCAIRO libpangocairo-1.0-0.dll)
        FIND_FILE (LIBPANGOFT2 libpangoft2-1.0-0.dll)
        FIND_FILE (LIBGDKPIXBUF libgdk_pixbuf-2.0-0.dll)
        FIND_FILE (LIBGDK libgdk-win32-2.0-0.dll)
        FIND_FILE (LIBGLIB libglib-2.0-0.dll)
        FIND_FILE (LIBGMODULE libgmodule-2.0-0.dll)
        FIND_FILE (LIBPANGO libpango-1.0-0.dll)
        FIND_FILE (LIBGTHREAD libgthread-2.0-0.dll)
        FIND_FILE (LIBPNG libpng14-14.dll)
        FIND_FILE (LIBGTK libgtk-win32-2.0-0.dll)
        FIND_FILE (LIBGOBJECT libgobject-2.0-0.dll)
        FIND_FILE (LIBPANGOWIN32 libpangowin32-1.0-0.dll)
        FIND_FILE (LIBZLIB1 zlib1.dll)
        INSTALL (FILES
                ${LIBATK}
                ${LIBEXPAT}
                ${LIBFONTCONFIG}
                ${LIBGIO}
                ${LIBCAIRO}
                ${LIBPANGOCAIRO}
                ${LIBGDKPIXBUF}
                ${LIBGDK}
                ${LIBGLIB}
                ${LIBGMODULE}
                ${LIBPANGO}
                ${LIBGTHREAD}
                ${LIBPNG}
                ${LIBGTK}
                ${LIBGOBJECT}
                ${LIBPANGOFT2}
                ${LIBPANGOWIN32}
                ${LIBZLIB1}
                DESTINATION .)
        INSTALL (FILES apvlvrc.example DESTINATION "."
                 RENAME _apvlvrc)
        INSTALL (FILES apvlvrc.example Startup.pdf DESTINATION ${DOCDIR})
        INSTALL (DIRECTORY icons DESTINATION ${DOCDIR}
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
        SET (CPACK_GENERATOR RPM)
        SET (CPACK_RPM_PACKAGE_REQUIRES "gtk2 >= 2.10.4")
ELSE (UNIX)
        SET (CPACK_GENERATOR NSIS)
        SET (CPACK_NSIS_CONTACT "Alf <naihe2010@gmail.com>")
        SET (CPACK_NSIS_EXTRA_INSTALL_COMMANDS "CreateShortCut '\$DESKTOP\\\\apvlv.lnk' '\$INSTDIR\\\\apvlv.exe'")
        SET (CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete '\$DESKTOP\\\\gsftc.lnk'")
ENDIF (UNIX)

INCLUDE(CPack)
