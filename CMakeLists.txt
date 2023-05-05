CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_EXPORT_COMPILE_COMMANDS 1)

PROJECT(apvlv)

SET(PROJECT_MAJOR "0")
SET(PROJECT_MINOR "4")
SET(PROJECT_PATCH "0")
FIND_PACKAGE(Git)
IF (GIT_FOUND)
    EXEC_PROGRAM(${GIT_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}
            ARGS "log -1 --format='%h'"
            OUTPUT_VARIABLE VERSION_HASH
            RETURN_VALUE RETURN)

    IF (RETURN EQUAL 0)
        MESSAGE(STATUS "Configure git: ${VERSION_HASH}")
        SET(PROJECT_PATCH ${VERSION_HASH})
    ENDIF (RETURN EQUAL 0)
ENDIF (GIT_FOUND)

SET(PACKAGE_VERSION "${PROJECT_MAJOR}.${PROJECT_MINOR}.${PROJECT_PATCH}")

IF (WIN32)
    ADD_DEFINITIONS(-DDOCDIR=".")
ELSE (WIN32)
    SET(SYSCONFDIR
            "/etc"
            CACHE PATH
            "Directory for system configuration of apvlv [/etc]"
            )
    ADD_DEFINITIONS(-DSYSCONFDIR="${SYSCONFDIR}")

    SET(DOCDIR
            "${CMAKE_INSTALL_PREFIX}/share/doc/apvlv"
            CACHE PATH
            "Directory for documentations of apvlv [PREFIX/share/doc/apvlv]"
            )
    ADD_DEFINITIONS(-DDOCDIR="${DOCDIR}")

    SET(LIBDIR
            "${CMAKE_INSTALL_PREFIX}/lib/apvlv"
            CACHE PATH
            "Directory for lib of apvlv [PREFIX/lib/apvlv]"
            )
    ADD_DEFINITIONS(-DLIBDIR="${LIBDIR}")

    SET(PIXMAPDIR
            "${CMAKE_INSTALL_PREFIX}/share/pixmaps/apvlv"
            CACHE PATH
            "Directory for pixmaps of apvlv [PREFIX/share/pixmaps/apvlv]"
            )
    ADD_DEFINITIONS(-DPIXMAPDIR="${PIXMAPDIR}")

    SET(APPLICATIONSDIR
            "${CMAKE_INSTALL_PREFIX}/share/applications"
            CACHE PATH
            "Directory for desktop file of apvlv [PREFIX/share/applications]"
            )
ENDIF (WIN32)

ADD_DEFINITIONS(-DPACKAGE_NAME="apvlv"
        -DPACKAGE_VERSION="${PACKAGE_VERSION}"
        -DPACKAGE_BUGREPORT="Alf <naihe2010@126.com>"
        -DRELEASE="rel"
        )

ADD_SUBDIRECTORY(src)

IF (WIN32)
    INSTALL(FILES apvlvrc.example DESTINATION "."
            RENAME _apvlvrc)
    INSTALL(FILES apvlvrc.example Startup.pdf main_menubar.glade DESTINATION ".")
    INSTALL(DIRECTORY icons DESTINATION ".")
    INSTALL(DIRECTORY ${CMAKE_HOME_DIRECTORY}/win32/poppler-data DESTINATION ".")
ELSE (WIN32)
    ADD_CUSTOM_TARGET(Startup.pdf
            ALL
            COMMAND "man" "-t" "${CMAKE_SOURCE_DIR}/apvlv.1" "|" "ps2pdf" "-" "${CMAKE_SOURCE_DIR}/Startup.pdf"
            DEPENDS apvlv.1)
    INSTALL(FILES apvlvrc.example DESTINATION ${SYSCONFDIR}
            RENAME apvlvrc)
    INSTALL(FILES apvlvrc.example Startup.pdf main_menubar.glade DESTINATION ${DOCDIR})
    INSTALL(DIRECTORY icons DESTINATION ${DOCDIR})
    INSTALL(FILES apvlv.1 TYPE MAN)
    INSTALL(FILES apvlv.desktop DESTINATION ${APPLICATIONSDIR})
ENDIF (WIN32)

SET(CPACK_PACKAGE_VENDOR "Alf")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "apvlv - Alf's PDF/DJVU/EPUB Viewer like Vim")
SET(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_PATCH})

IF (UNIX)
    SET(CPACK_SET_DESTDIR ON)
    SET(CPACK_PACKAGE_CONTACT "Alf <naihe2010@126.com>")
    SET(CPACK_GENERATOR DEB)
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS
            "libpoppler-glib8 (>= 0.8.7-4), poppler-data (>= 0.2.0-2)")
    IF (EXISTS "/etc/redhat-release")
        SET(CPACK_GENERATOR RPM)
        SET(CPACK_RPM_PACKAGE_REQUIRES "gtk3")
    ENDIF ()
    SET(CPACK_SOURCE_GENERATOR TGZ)
    SET(CPACK_SOURCE_IGNORE_FILES
            ${CMAKE_BINARY_DIR}
            ".git"
            ".gitignore"
            "win32"
            "~$"
            )
ELSE (UNIX)
    SET(CPACK_GENERATOR NSIS)
    SET(CPACK_NSIS_CONTACT "Alf <naihe2010@126.com>")
    SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "CreateShortCut '\$DESKTOP\\\\apvlv.lnk' '\$INSTDIR\\\\apvlv.exe'")
    SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete '\$DESKTOP\\\\gsftc.lnk'")
ENDIF (UNIX)

INCLUDE(CPack)
