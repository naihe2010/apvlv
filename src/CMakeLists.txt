CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

ADD_DEFINITIONS (-DPACKAGE_NAME="apvlv"
                 -DPACKAGE_VERSION="${PROJECT_MAJOR}.${PROJECT_MINOR}.${PROJECT_PATCH}"
                 -DPACKAGE_BUGREPORT="Alf <naihe2010@gmail.com>"
                 -DRELEASE="rel"
                 -DPROJECT_MAJOR="${PROJECT_MAJOR}"
                 -DPROJECT_MINOR="${PROJECT_MINOR}"
                 -DPROJECT_PATCH="${PROJECT_PATCH}"
)

IF (WIN32)
        ADD_DEFINITIONS (-DDOCDIR=".")
ELSE (WIN32)
        ADD_DEFINITIONS (-DDOCDIR="${CMAKE_PREFIX}/share/apvlv/doc")
        ADD_DEFINITIONS (-DSYSCONFDIR="/etc")
        OPTION (APVLV_ENABLE_DEBUG "If build binary with debug infomations." OFF)
        SET (CMAKE_C_FLAGS "-Wall -fno-strict-aliasing")
        IF (APVLV_ENABLE_DEBUG)
                SET (CMAKE_C_FLAGS "-D_DEBUG -g ${CMAKE_C_FLAGS}")
        ELSE (APVLV_ENABLE_DEBUG)
                SET (CMAKE_C_FLAGS "-O2 ${CMAKE_C_FLAGS}")
        ENDIF (APVLV_ENABLE_DEBUG)
ENDIF (WIN32)

FIND_PACKAGE (PkgConfig)
PKG_CHECK_MODULES (GTK2 gtk+-2.0 REQUIRED)
PKG_CHECK_MODULES (GLIB2 glib-2.0 REQUIRED)
PKG_CHECK_MODULES (GTHREAD2 gthread-2.0 REQUIRED)
INCLUDE_DIRECTORIES (${GTK2_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS} ${GTHREAD2_INCLUDE_DIRS})
LINK_DIRECTORIES (${GTK2_LIBRARY_DIRS} 
                  ${GLIB2_LIBRARY_DIRS} 
                  ${GTHREAD2_LIBRARY_DIRS})

IF (NOT WIN32)
        PKG_CHECK_MODULES (POPPLER poppler-glib REQUIRED)
        INCLUDE_DIRECTORIES (${POPPLER_INCLUDE_DIRS})
        LINK_DIRECTORIES (${POPPLER_LIBRARY_DIRS})
ELSE (NOT WIN32)
	INCLUDE_DIRECTORIES (${CMAKE_HOME_DIRECTORY}/win32/poppler-src)
	LINK_DIRECTORIES (${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_HOME_DIRECTORY}/win32/freetype235)
        SET (POPPLER_LIBRARIES poppler.lib jpeg.lib freetype-static.lib)
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

IF (WIN32)
	ADD_SUBDIRECTORY (${CMAKE_HOME_DIRECTORY}/win32/poppler-src .)
        ADD_DEPENDENCIES (apvlv poppler)
ENDIF (WIN32)
