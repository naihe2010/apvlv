ADD_COMPILE_OPTIONS(-std=c++17)

IF (WIN32)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ELSE (WIN32)
    SET(CMAKE_CXX_FLAGS "-Wall -fno-strict-aliasing")

    OPTION(APVLV_ENABLE_DEBUG "If build binary with debug information." OFF)
    IF (APVLV_ENABLE_DEBUG)
        SET(CMAKE_CXX_FLAGS "-D_DEBUG -g ${CMAKE_CXX_FLAGS}")
    ELSE (APVLV_ENABLE_DEBUG)
        SET(CMAKE_CXX_FLAGS "-O2 ${CMAKE_CXX_FLAGS}")
    ENDIF (APVLV_ENABLE_DEBUG)
ENDIF (WIN32)

FIND_PACKAGE(PkgConfig)

PKG_CHECK_MODULES(FREETYPE2 freetype2 REQUIRED)
PKG_CHECK_MODULES(GTK gtk+-3.0)
PKG_CHECK_MODULES(GLIB2 glib-2.0 REQUIRED)
PKG_CHECK_MODULES(LIBXML libxml-2.0 REQUIRED)
PKG_CHECK_MODULES(WEBKIT webkit2gtk-4.1)
if (NOT WEBKIT_FOUND)
    PKG_CHECK_MODULES(WEBKIT webkit2gtk-4.0 REQUIRED)
endif ()
INCLUDE_DIRECTORIES(${FREETYPE2_INCLUDE_DIRS}
        ${GTK_INCLUDE_DIRS}
        ${GLIB2_INCLUDE_DIRS}
        ${LIBXML_INCLUDE_DIRS}
        ${WEBKIT_INCLUDE_DIRS})
LINK_DIRECTORIES(${FREETYPE2_LIBRARY_DIRS}
        ${GTK_LIBRARY_DIRS}
        ${GLIB2_LIBRARY_DIRS}
        ${LIBXML_LIBRARY_DIRS}
        ${WEBKIT_LIBRARY_DIRS})

IF (WIN32)
    SET(POPPLER_DIR ${CMAKE_HOME_DIRECTORY}/win32/poppler-src)

    INCLUDE_DIRECTORIES(${POPPLER_DIR})
    LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/poppler-build)
    SET(APVLV_REQ_LIBRARIES poppler.lib freetype.lib)

    ADD_SUBDIRECTORY(${POPPLER_DIR} ${CMAKE_BINARY_DIR}/poppler-build)
ELSE (WIN32)
    PKG_CHECK_MODULES(POPPLER poppler-glib REQUIRED)
    INCLUDE_DIRECTORIES(${POPPLER_INCLUDE_DIRS})
    LINK_DIRECTORIES(${POPPLER_LIBRARY_DIRS})
    SET(APVLV_REQ_LIBRARIES ${POPPLER_LIBRARIES})
ENDIF (WIN32)

SET(HEADERS
        ApvlvCmds.h
        ApvlvCore.h
        ApvlvDoc.h
        ApvlvFile.h
        ApvlvPdf.h
        ApvlvInfo.h
        ApvlvMenuAndTool.h
        ApvlvParams.h
        ApvlvUtil.h
        ApvlvView.h
        ApvlvWindow.h
        ApvlvCompletion.h
        ApvlvHtm.h
        ApvlvEpub.h
        ApvlvTxt.h
        ApvlvFb2.h
        ApvlvContent.h
        ApvlvLab.h
)

SET(SOURCES
        ApvlvCmds.cc
        ApvlvCore.cc
        ApvlvDoc.cc
        ApvlvFile.cc
        ApvlvPdf.cc
        ApvlvInfo.cc
        ApvlvMenuAndTool.cc
        ApvlvParams.cc
        ApvlvUtil.cc
        ApvlvView.cc
        ApvlvWindow.cc
        ApvlvCompletion.cc
        ApvlvHtm.cc
        ApvlvEpub.cc
        ApvlvTxt.cc
        ApvlvFb2.cc
        ApvlvContent.cc
        ApvlvLab.cc
        main.cc
)

OPTION(APVLV_WITH_DJVU "If build apvlv with djvu format support." OFF)
IF (APVLV_WITH_DJVU)
    ADD_DEFINITIONS(-DAPVLV_WITH_DJVU)

    IF (WIN32)
        SET(DJVULIBRE_DIR ${CMAKE_HOME_DIRECTORY}/win32/djvulibre)

        INCLUDE_DIRECTORIES(${DJVULIBRE_DIR}/include)
        LINK_DIRECTORIES(${DJVULIBRE_DIR})
        SET(APVLV_REQ_LIBRARIES libdjvulibre.lib
                ${APVLV_REQ_LIBRARIES})
    ELSE (WIN32)
        SET(APVLV_REQ_LIBRARIES -ldjvulibre
                ${APVLV_REQ_LIBRARIES})
    ENDIF (WIN32)

    SET(HEADERS ${HEADERS} ApvlvDjvu.h)
    SET(SOURCES ${SOURCES} ApvlvDjvu.cc)
ENDIF (APVLV_WITH_DJVU)

ADD_EXECUTABLE(apvlv ${HEADERS} ${SOURCES})

TARGET_LINK_LIBRARIES(apvlv
        ${GTK_LIBRARIES}
        ${GLIB2_LIBRARIES}
        ${GTHREAD2_LIBRARIES}
        ${WEBKIT_LIBRARIES}
        "-lepub"
        ${LIBXML_LIBRARIES}
        ${APVLV_REQ_LIBRARIES})

IF (WIN32)
    ADD_DEPENDENCIES(apvlv poppler)
ENDIF (WIN32)

IF (WIN32)
    INSTALL(TARGETS apvlv DESTINATION .)
    FIND_FILE(LIBATK libatk-1.0-0.dll)
    FIND_FILE(LIBEXPAT1 libexpat-1.dll)
    FIND_FILE(LIBFONTCONFIG1 libfontconfig-1.dll)
    FIND_FILE(LIBGIO libgio-2.0-0.dll)
    FIND_FILE(LIBCAIRO libcairo-2.dll)
    FIND_FILE(LIBPANGOCAIRO libpangocairo-1.0-0.dll)
    FIND_FILE(LIBPANGOFT2 libpangoft2-1.0-0.dll)
    FIND_FILE(LIBGDKPIXBUF libgdk_pixbuf-2.0-0.dll)
    FIND_FILE(LIBGDK libgdk-win32-2.0-0.dll)
    FIND_FILE(LIBGLIB libglib-2.0-0.dll)
    FIND_FILE(LIBGMODULE libgmodule-2.0-0.dll)
    FIND_FILE(LIBPANGO libpango-1.0-0.dll)
    FIND_FILE(LIBGTHREAD libgthread-2.0-0.dll)
    FIND_FILE(LIBPNG libpng14-14.dll)
    FIND_FILE(LIBGTK libgtk-win32-2.0-0.dll)
    FIND_FILE(LIBGOBJECT libgobject-2.0-0.dll)
    FIND_FILE(LIBPANGOWIN32 libpangowin32-1.0-0.dll)
    FIND_FILE(LIBZLIB1 zlib1.dll)
    FIND_FILE(FREETYPE6 freetype6.dll)
    INSTALL(FILES
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
            DESTINATION .)
    IF (APVLV_WITH_DJVU)
        INSTALL(FILES
                ${DJVULIBRE_DIR}/libdjvulibre.dll
                ${DJVULIBRE_DIR}/libjpeg.dll
                DESTINATION .)
    ENDIF (APVLV_WITH_DJVU)
ELSE (WIN32)
    INSTALL(TARGETS apvlv DESTINATION bin)
ENDIF (WIN32)
