IF (WIN32)
        ADD_DEFINITIONS (-D_CRT_SECURE_NO_WARNINGS)
ELSE (WIN32)
        OPTION (APVLV_ENABLE_DEBUG "If build binary with debug infomations." OFF)
        SET (CMAKE_CXX_FLAGS "-Wall -fno-strict-aliasing")
        IF (APVLV_ENABLE_DEBUG)
                SET (CMAKE_CXX_FLAGS "-D_DEBUG -g ${CMAKE_CXX_FLAGS}")
        ELSE (APVLV_ENABLE_DEBUG)
                SET (CMAKE_CXX_FLAGS "-O2 ${CMAKE_CXX_FLAGS}")
        ENDIF (APVLV_ENABLE_DEBUG)
ENDIF (WIN32)

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

IF (NOT WIN32)
        PKG_CHECK_MODULES (POPPLER poppler-glib REQUIRED)
        INCLUDE_DIRECTORIES (${POPPLER_INCLUDE_DIRS})
        LINK_DIRECTORIES (${POPPLER_LIBRARY_DIRS})
ELSE (NOT WIN32)
        INCLUDE_DIRECTORIES (${CMAKE_HOME_DIRECTORY}/win32/poppler-src 
                             ${CMAKE_HOME_DIRECTORY}/win32/libumd-src
        )
        LINK_DIRECTORIES (${CMAKE_BINARY_DIR}/poppler-out
                          ${CMAKE_BINARY_DIR}/libumd-out 
                          ${CMAKE_HOME_DIRECTORY}/win32/libumd-src/libiconv-1.9.2-1-lib/lib
                          ${CMAKE_HOME_DIRECTORY}/win32/libumd-src/zlib125-dll/lib)
        SET (POPPLER_LIBRARIES poppler.lib freetype.lib)
ENDIF (NOT WIN32)

SET (HEADERS
     ApvlvCmds.hpp
     ApvlvCore.hpp
     ApvlvDir.hpp
     ApvlvDoc.hpp
     ApvlvFile.hpp
     ApvlvInfo.hpp
     ApvlvMenu.hpp
     ApvlvParams.hpp
     ApvlvUtil.hpp
     ApvlvView.hpp
     ApvlvWindow.hpp
)

SET (SOURCES
     ApvlvCmds.cpp
     ApvlvCore.cpp
     ApvlvDir.cpp
     ApvlvDoc.cpp
     ApvlvFile.cpp
     ApvlvInfo.cpp
     ApvlvMenu.cpp
     ApvlvParams.cpp
     ApvlvUtil.cpp
     ApvlvView.cpp
     ApvlvWindow.cpp
     main.cpp
)

ADD_EXECUTABLE (apvlv ${HEADERS} ${SOURCES})
TARGET_LINK_LIBRARIES (apvlv
                       ${GTK2_LIBRARIES} 
                       ${GLIB2_LIBRARIES} 
                       ${GTHREAD2_LIBRARIES}
                       ${POPPLER_LIBRARIES}) 

OPTION (APVLV_WITH_DJVU "If build apvlv with djvu format support." ON)
IF (APVLV_WITH_DJVU)
        ADD_DEFINITIONS (-DHAVE_LIBDJVU)
        TARGET_LINK_LIBRARIES (apvlv -ldjvulibre)
ENDIF (APVLV_WITH_DJVU)

OPTION (APVLV_WITH_UMD "If build binary with umd format support." ON)
IF (APVLV_WITH_UMD)
        ADD_DEFINITIONS (-DHAVE_LIBUMD)
        IF (NOT WIN32)
        TARGET_LINK_LIBRARIES (apvlv -lumd)
        ENDIF (NOT WIN32)
ENDIF (APVLV_WITH_UMD)

IF (WIN32)
        CMAKE_POLICY (SET CMP0013 NEW)
        ADD_SUBDIRECTORY (${CMAKE_HOME_DIRECTORY}/win32/poppler-src ${CMAKE_BINARY_DIR}/poppler-out)
        ADD_DEPENDENCIES (apvlv poppler)

        IF (APVLV_WITH_UMD)
                ADD_SUBDIRECTORY (${CMAKE_HOME_DIRECTORY}/win32/libumd-src ${CMAKE_BINARY_DIR}/libumd-out)
                ADD_DEPENDENCIES (apvlv umd)
                TARGET_LINK_LIBRARIES (apvlv umd.lib libiconv.lib zdll.lib)
        ENDIF (APVLV_WITH_UMD)
ENDIF (WIN32)

IF (WIN32)
        INSTALL (TARGETS apvlv DESTINATION .)
        FIND_FILE (LIBATK libatk-1.0-0.dll)
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
        INSTALL (FILES
                ${CMAKE_HOME_DIRECTORY}/win32/libumd-src/libiconv-1.9.2-1-lib/lib/libiconv2.dll
                ${CMAKE_HOME_DIRECTORY}/win32/libumd-src/libiconv-1.9.2-1-lib/lib/libintl3.dll
                DESTINATION .)
ELSE (WIN32)
        INSTALL (TARGETS apvlv DESTINATION bin)
ENDIF (WIN32)
