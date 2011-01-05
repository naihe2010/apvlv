CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

PROJECT (libumd)

IF (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
        MESSAGE (FATAL_ERROR "LibUMD requires a out of source Build. Please create a separate binary directory and run CMake there")
ENDIF (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

INCLUDE (CheckTypeSize)

CHECK_TYPE_SIZE ("unsigned char" UCHAR_SIZE)
IF (NOT ${UCHAR_SIZE} EQUAL 1)
     MESSAGE (FATAL_ERROR "sizeof unsigned char is not matched, abort")
ENDIF (NOT ${UCHAR_SIZE} EQUAL 1)

CHECK_TYPE_SIZE ("unsigned short" USHORT_SIZE)
IF (NOT ${USHORT_SIZE} EQUAL 2)
     MESSAGE (FATAL_ERROR "sizeof unsigned short is not matched, abort")
ENDIF (NOT ${USHORT_SIZE} EQUAL 2)

CHECK_TYPE_SIZE ("unsigned long" ULONG_SIZE)
IF (NOT ${ULONG_SIZE} EQUAL 4)
     MESSAGE (FATAL_ERROR "sizeof unsigned long is not matched, abort")
ENDIF (NOT ${ULONG_SIZE} EQUAL 4)

SET (PROJECT_MAJOR      "0")
SET (PROJECT_MINOR      "0")
SET (PROJECT_PATCH      "1")
SET (PROJECT_VERSION ${PROJECT_MAJOR}.${PROJECT_MINOR}.${PROJECT_PATCH})

ADD_DEFINITIONS (-DPROJECT_MAJOR="${PROJECT_MAJOR}"
                 -DPROJECT_MINOR="${PROJECT_MINOR}"
                 -DPROJECT_PATCH="${PROJECT_PATCH}")

IF (WIN32)
        ADD_DEFINITIONS (-DWIN32 
                         -D_CRT_SECURE_NO_WARNINGS)
ELSE (WIN32)
        SET (CMAKE_C_FLAGS "-Wall -fno-strict-aliasing")

        OPTION (LIBUMD_ENABLE_DEBUG "If build binary with debug infomations." OFF)
        IF (LIBUMD_ENABLE_DEBUG)
                SET (CMAKE_C_FLAGS "-D_DEBUG -g ${CMAKE_C_FLAGS}")
        ELSE (LIBUMD_ENABLE_DEBUG)
                SET (CMAKE_C_FLAGS "-O2 ${CMAKE_C_FLAGS}")
        ENDIF (LIBUMD_ENABLE_DEBUG)

        OPTION (LIBUMD_WITH_MUDFLAP "If build binary with mudflap." OFF)
        if (LIBUMD_WITH_MUDFLAP)
                SET (CMAKE_C_FLAGS "-fmudflap -lmudflap ${CMAKE_C_FLAGS}")
        ENDIF (LIBUMD_WITH_MUDFLAP)
ENDIF (WIN32)

SET (HEADERS
     umd.h
     umd_internals.h
)

SET (SOURCES
     umd.c
     umd_page.c
)

IF (WIN32)
	CMAKE_POLICY (SET CMP0015 NEW)
	INCLUDE_DIRECTORIES (
                "libiconv-1.9.2-1-lib/include"
		"zlib125-dll/include"
		)
	LINK_DIRECTORIES (
                "libiconv-1.9.2-1-lib/lib"
		"zlib125-dll/lib"
		)
ENDIF (WIN32)

FIND_PACKAGE (PkgConfig)
PKG_CHECK_MODULES (CAIRO cairo REQUIRED)
PKG_CHECK_MODULES (PANGO pangocairo REQUIRED)
INCLUDE_DIRECTORIES (${CAIRO_INCLUDE_DIRS} ${PANGO_INCLUDE_DIRS})
LINK_DIRECTORIES (${CAIRO_LIBRARY_DIRS} ${PANGO_LIBRARY_DIRS} ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

SET (REQUIRED_LIBRARIES
     ${CAIRO_LIBRARIES}
     ${PANGO_LIBRARIES})

IF (WIN32)
	ADD_LIBRARY (umd ${HEADERS} ${SOURCES})

        SET (REQUIRED_LIBRARIES ${REQUIRED_LIBRARIES}
             libiconv.lib zdll.lib)
	TARGET_LINK_LIBRARIES (umd ${REQUIRED_LIBRARIES})
ELSE (WIN32)
	ADD_LIBRARY (umd SHARED ${HEADERS} ${SOURCES})

        SET (REQUIRED_LIBRARIES ${REQUIRED_LIBRARIES}
             -lz)
ENDIF (WIN32)

IF (NOT WIN32)
SET (DOC_OUTPUT_DIR     "${CMAKE_SOURCE_DIR}/documentation")

ADD_CUSTOM_COMMAND (OUTPUT
                    ${DOC_OUTPUT_DIR}/html/index.html
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    COMMAND doxygen
                    DEPENDS
                    ${HEADERS}
                    ${SOURCES}
                    ${CMAKE_SOURCE_DIR}/Doxyfile)
ADD_CUSTOM_TARGET (doc DEPENDS ${DOC_OUTPUT_DIR}/html/index.html)

ADD_DEPENDENCIES (umd doc)
ENDIF (NOT WIN32)

ADD_EXECUTABLE (umd2txt umd2txt.c)
IF (WIN32)
        TARGET_LINK_LIBRARIES (umd2txt ${REQUIRED_LIBRARIES} umd.lib)
ELSE (WIN32)
        TARGET_LINK_LIBRARIES (umd2txt ${REQUIRED_LIBRARIES} -lumd)
ENDIF (WIN32)

ADD_EXECUTABLE (umdtest umdtest.c)
OPTION (ENABLE_GTK      "If enable GTK+ support for umdtest" on)
IF (ENABLE_GTK)
        ADD_DEFINITIONS (-DENABLE_GTK)
        FIND_PACKAGE (PkgConfig)
        PKG_CHECK_MODULES (GTK2 gtk+-2.0 REQUIRED)
        INCLUDE_DIRECTORIES (${GTK2_INCLUDE_DIRS})
        LINK_DIRECTORIES (${GTK2_LIBRARY_DIRS})
        TARGET_LINK_LIBRARIES (umdtest ${GTK2_LIBRARIES})
ENDIF (ENABLE_GTK)
IF (WIN32)
        TARGET_LINK_LIBRARIES (umdtest ${REQUIRED_LIBRARIES} umd.lib)
ELSE (WIN32)
        TARGET_LINK_LIBRARIES (umdtest ${REQUIRED_LIBRARIES} -lumd)
ENDIF (WIN32)

INSTALL (FILES ${CMAKE_SOURCE_DIR}/umd.h DESTINATION include)
INSTALL (TARGETS umd DESTINATION lib)
INSTALL (TARGETS umd2txt DESTINATION bin)
IF (WIN32)
        INSTALL (FILES 
                 "libiconv-1.9.2-1-lib/lib/libiconv2.dll"
                 "libiconv-1.9.2-1-lib/lib/libintl3.dll"
                 "zlib125-dll/lib/zlib1.dll"
                 ${LIBICONV} DESTINATION bin) 
ELSE (WIN32)
        INSTALL (DIRECTORY ${DOC_OUTPUT_DIR}/html
                 DESTINATION share/doc/libumd
                 PATTERN ".svn" EXCLUDE)
        FILE (GLOB MANS ${DOC_OUTPUT_DIR}/man/man3/*.3)
        INSTALL (FILES ${MANS} DESTINATION share/man/man3)
ENDIF (WIN32)

SET (CPACK_PACKAGE_VENDOR "Alf")
SET (CPACK_PACKAGE_DESCRIPTION_SUMMARY "libumd - Library for parse UMD files")
SET (CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_MAJOR})
SET (CPACK_PACKAGE_VERSION_MINOR ${PROJECT_MINOR})
SET (CPACK_PACKAGE_VERSION_PATCH ${PROJECT_PATCH})

IF (UNIX)
# source tar.bz2 package
        SET (CPACK_SOURCE_GENERATOR TBZ2)
        SET (CPACK_SOURCE_IGNORE_FILES ${CMAKE_BINARY_DIR} ".svn")
        SET (CPACK_SOURCE_PACKAGE_FILE_NAME
        ${CMAKE_PROJECT_NAME}-${PROJECT_VERSION})

        ADD_CUSTOM_TARGET (rpm COMMAND make package)

# install RPM package
        SET (CPACK_SET_DESTDIR ON)
        SET (CPACK_GENERATOR RPM)
        SET (CPACK_PACKAGE_FILE_NAME
        ${CMAKE_PROJECT_NAME}-${PROJECT_VERSION})

        ADD_CUSTOM_TARGET (dist COMMAND make package_source)
ELSE (UNIX)
        SET (CPACK_GENERATOR NSIS)
        SET (CPACK_NSIS_CONTACT "Alf <naihe2010@gmail.com>")
ENDIF (UNIX)

INCLUDE(CPack)
