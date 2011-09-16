IF (WIN32)
        ADD_DEFINITIONS (-D_CRT_SECURE_NO_WARNINGS)
ELSE (WIN32)
        SET (CMAKE_CXX_FLAGS "-Wall -fno-strict-aliasing")

        OPTION (APVLV_ENABLE_DEBUG "If build binary with debug infomations." OFF)
        IF (APVLV_ENABLE_DEBUG)
                SET (CMAKE_CXX_FLAGS "-D_DEBUG -g ${CMAKE_CXX_FLAGS}")
        ELSE (APVLV_ENABLE_DEBUG)
                SET (CMAKE_CXX_FLAGS "-O2 ${CMAKE_CXX_FLAGS}")
        ENDIF (APVLV_ENABLE_DEBUG)
ENDIF (WIN32)

FIND_PACKAGE (PkgConfig)

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

PKG_CHECK_MODULES (FREETYPE2 freetype2 REQUIRED)
PKG_CHECK_MODULES (GTK2 gtk+-2.0 REQUIRED)
PKG_CHECK_MODULES (GLIB2 glib-2.0 REQUIRED)
PKG_CHECK_MODULES (GTHREAD2 gthread-2.0 REQUIRED)
PKG_CHECK_MODULES (WEBKIT1 webkit-1.0 QUIET)
INCLUDE_DIRECTORIES (${FREETYPE2_INCLUDE_DIRS} 
                     ${GTK2_INCLUDE_DIRS} 
                     ${GLIB2_INCLUDE_DIRS} 
                     ${GTHREAD2_INCLUDE_DIRS}
                     ${WEBKIT1_INCLUDE_DIRS})
LINK_DIRECTORIES (${FREETYPE2_LIBRARY_DIRS}
                  ${GTK2_LIBRARY_DIRS} 
                  ${GLIB2_LIBRARY_DIRS} 
                  ${GTHREAD2_LIBRARY_DIRS}
                  ${WEBKIT1_LIBRARY_DIRS})

SET (HEADERS
     ApvlvCmds.h
     ApvlvCore.h
     ApvlvDir.h
     ApvlvDoc.h
     ApvlvFile.h
     ApvlvPdf.h
     ApvlvInfo.h
     ApvlvMenu.h
     ApvlvParams.h
     ApvlvUtil.h
     ApvlvView.h
     ApvlvWindow.h
)

SET (SOURCES
     ApvlvCmds.cc
     ApvlvCore.cc
     ApvlvDir.cc
     ApvlvDoc.cc
     ApvlvFile.cc
     ApvlvPdf.cc
     ApvlvInfo.cc
     ApvlvMenu.cc
     ApvlvParams.cc
     ApvlvUtil.cc
     ApvlvView.cc
     ApvlvWindow.cc
     main.cc
)

OPTION (APVLV_WITH_DJVU "If build apvlv with djvu format support." OFF)
IF (APVLV_WITH_DJVU)
        ADD_DEFINITIONS (-DAPVLV_WITH_DJVU)

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

IF (APVLV_WITH_DJVU)
     SET (HEADERS ${HEADERS} ApvlvDjvu.h)
     SET (SOURCES ${SOURCES} ApvlvDjvu.cc)
ENDIF (APVLV_WITH_DJVU)

OPTION (APVLV_WITH_UMD "If build binary with umd format support." OFF)
IF (APVLV_WITH_UMD)
        ADD_DEFINITIONS (-DAPVLV_WITH_UMD)

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

IF (APVLV_WITH_UMD)
     SET (HEADERS ${HEADERS} ApvlvUmd.h)
     SET (SOURCES ${SOURCES} ApvlvUmd.cc)
ENDIF (APVLV_WITH_UMD)

OPTION (APVLV_WITH_HTML "If build binary with html format support." OFF)
IF (APVLV_WITH_HTML)
        ADD_DEFINITIONS (-DAPVLV_WITH_HTML)
ENDIF (APVLV_WITH_HTML)

IF (APVLV_WITH_HTML)
     SET (HEADERS ${HEADERS} ApvlvHtm.h)
     SET (SOURCES ${SOURCES} ApvlvHtm.cc)
ENDIF (APVLV_WITH_HTML)

ADD_EXECUTABLE (apvlv ${HEADERS} ${SOURCES})

TARGET_LINK_LIBRARIES (apvlv
                       ${GTK2_LIBRARIES} 
                       ${GLIB2_LIBRARIES} 
                       ${GTHREAD2_LIBRARIES}
                       ${WEBKIT1_LIBRARIES}
                       ${APVLV_REQ_LIBRARIES})

IF (WIN32)
        ADD_DEPENDENCIES (apvlv poppler)
        IF (APVLV_WITH_UMD)
                ADD_DEPENDENCIES (apvlv umd)
        ENDIF (APVLV_WITH_UMD)
ENDIF (WIN32)

IF (WIN32)
        INSTALL (TARGETS apvlv DESTINATION .)
        FIND_FILE (LIBATK libatk-1.0-0.dll)
        FIND_FILE (LIBEXPAT1 libexpat-1.dll)
        FIND_FILE (LIBFONTCONFIG1 libfontconfig-1.dll)
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
        FIND_FILE (FREETYPE6 freetype6.dll)
        INSTALL (FILES
                ${LIBATK}
                ${LIBEXPAT1}
                ${LIBFONTCONFIG1}
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
                ${FREETYPE6}
                ${DJVULIBRE_DIR}/libdjvulibre.dll
                ${DJVULIBRE_DIR}/libjpeg.dll
                DESTINATION .)
ELSE (WIN32)
        INSTALL (TARGETS apvlv DESTINATION bin)
ENDIF (WIN32)
