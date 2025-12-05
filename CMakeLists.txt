cmake_minimum_required(VERSION 3.16)
project(apvlv VERSION 0.7.0)

# vcpkg integration
if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Include cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Options)
include(CompileFlags)
include(Dependencies)

# Version info
find_package(Git)
if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE VERSION_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(VERSION_HASH)
        message(STATUS "Git hash: ${VERSION_HASH}")
        set(PROJECT_PATCH ${VERSION_HASH})
    endif()
endif()

# Package definitions
if(WIN32)
    # On Windows, use AppData\Local or install directory
    set(SYSCONFDIR "$ENV{LOCALAPPDATA}/apvlv" CACHE PATH "System config directory")
else()
    set(SYSCONFDIR "/etc" CACHE PATH "System config directory")
endif()
add_definitions(-DSYSCONFDIR="${SYSCONFDIR}")

add_definitions(-DPACKAGE_NAME="apvlv"
    -DPACKAGE_VERSION="${PROJECT_VERSION}"
    -DPACKAGE_BUGREPORT="Alf <naihe2010@126.com>"
    -DRELEASE="rel"
)

ADD_SUBDIRECTORY(src)

IF (NOT WIN32)
    ADD_CUSTOM_TARGET(CopyCompileCommands ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
            COMMENT "Copying compile_commands.json to ${CMAKE_SOURCE_DIR}"
            DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
    )
ENDIF ()

INSTALL(DIRECTORY share DESTINATION ".")
IF (WIN32)
    INSTALL(FILES apvlvrc.example DESTINATION ".")
ELSE (WIN32)
    ADD_CUSTOM_TARGET(Startup.pdf
            ALL
            COMMAND "man" "-t" "${CMAKE_SOURCE_DIR}/apvlv.1" "|" "ps2pdf" "-" "${CMAKE_SOURCE_DIR}/share/doc/apvlv/Startup.pdf"
            DEPENDS apvlv.1)
    INSTALL(FILES apvlvrc.example DESTINATION ${SYSCONFDIR})
    INSTALL(FILES apvlv.1 TYPE MAN)
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
            "qt6 quazip cmark")
    IF (EXISTS "/etc/redhat-release")
        SET(CPACK_GENERATOR RPM)
        SET(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtwebengine quazip-qt6 cmark")
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
    SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "CreateShortCut '\$DESKTOP\\\\apvlv.lnk' '\$INSTDIR\\\\bin\\\\apvlv.exe'")
    SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete '\$DESKTOP\\\\apvlv.lnk'")
ENDIF (UNIX)

INCLUDE(CPack)
