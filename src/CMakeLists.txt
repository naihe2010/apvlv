# compile options
IF (WIN32)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ELSE ()
    SET(CMAKE_CXX_FLAGS "-std=c++17 -Wall -fno-strict-aliasing")

    IF (${CMAKE_BUILD_TYPE} STREQUAL Debug)
        SET(CMAKE_CXX_FLAGS "-D_DEBUG -g ${CMAKE_CXX_FLAGS}")
    ELSE ()
        SET(CMAKE_CXX_FLAGS "-O2 ${CMAKE_CXX_FLAGS}")
    ENDIF ()
ENDIF ()
MESSAGE("-- BUILD TYPE: ${CMAKE_BUILD_TYPE}")
MESSAGE("-- CXX FLAGS: ${CMAKE_CXX_FLAGS}")

SET(APVLV_WITH_POPPLER ON CACHE BOOL "if has poppler-qt6 library")

IF (WIN32)
    SET(Qt6_DIR "C:/Qt/6.7.2/msvc2019_64/lib/cmake/Qt6" CACHE STRING "Qt6config.cmake directory")
    SET(Qt6QmlTools_DIR "C:/Qt/6.7.2/msvc2019_64/lib/cmake/Qt6QmlTools" CACHE STRING "Qt6QmlToolsconfig.cmake directory")
    SET(QUAZIP_INCLUDE_DIRS "C:/Qt/6.7.2/msvc2019_64/include/QuaZip-Qt6-1.4/quazip;C:/Qt/6.7.2/msvc2019_64/include" CACHE STRING "Quazip include directory")
    SET(APVLV_WITH_POPPLER OFF)
    SET(QUAZIP_LIBRARY_DIRS "C:/Qt/6.7.2/msvc2019_64/lib" CACHE STRING "Quazip lib directory")
    SET(QUAZIP_LIBRARIES "C:/Qt/6.7.2/msvc2019_64/lib/quazip1-qt6.lib" CACHE STRING "Quazip library file")
    CMAKE_POLICY(SET CMP0010 NEW)
    CMAKE_POLICY(SET CMP0087 NEW)
ELSE ()
    FIND_PACKAGE(PkgConfig)
    PKG_CHECK_MODULES(POPPLER poppler-qt6 REQUIRED)
    PKG_CHECK_MODULES(QUAZIP quazip1-qt6 REQUIRED)
ENDIF ()

FIND_PACKAGE(Qt6 NAMES Qt6 COMPONENTS Core Gui Widgets WebEngineWidgets Pdf Xml PrintSupport REQUIRED)

SET(Qt_INCLUDE_DIRS
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Gui_INCLUDE_DIRS}
        ${Qt6Widgets_INCLUDE_DIRS}
        ${Qt6WebEngineWidgets_INCLUDE_DIRS}
        ${Qt6Pdf_INCLUDE_DIRS}
        ${Qt6Xml_INCLUDE_DIRS}
        ${Qt6PrintSupport_INCLUDE_DIRS})
SET(Qt_LIBRARIES Qt6::Core Qt6::Gui Qt6::Widgets Qt6::WebEngineWidgets Qt::Pdf Qt6::Xml Qt6::PrintSupport)
GET_FILENAME_COMPONENT(_qt_bin_dir "${WINDEPLOYQT_EXECUTABLE}" DIRECTORY)
MESSAGE("-- Qt6 binary directory: ${_qt_bin_dir}")

INCLUDE_DIRECTORIES(
        ${POPPLER_INCLUDE_DIRS}
        ${QUAZIP_INCLUDE_DIRS}
        ${Qt_INCLUDE_DIRS})
LINK_DIRECTORIES(
        ${POPPLER_LIBRARY_DIRS}
        ${QUAZIP_LIBRARY_DIRS}
        ${Qt_LIBRARY_DIRS})
SET(APVLV_REQ_LIBRARIES
        ${POPPLER_LIBRARIES}
        ${QUAZIP_LIBRARIES}
        ${Qt_LIBRARIES})

ADD_DEFINITIONS(-DQT_MESSAGELOGCONTEXT)

SET(HEADERS
        ApvlvCmds.h
        ApvlvCore.h
        ApvlvDoc.h
        ApvlvFile.h
        ApvlvInfo.h
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
        ApvlvLog.h
        ApvlvSearchDialog.h
)

SET(SOURCES
        ApvlvCmds.cc
        ApvlvCore.cc
        ApvlvDoc.cc
        ApvlvFile.cc
        ApvlvInfo.cc
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
        ApvlvLog.cc
        ApvlvSearchDialog.cc
        main.cc
)

IF (APVLV_WITH_POPPLER)
    ADD_DEFINITIONS(-DAPVLV_WITH_POPPLER)
    SET(HEADERS ${HEADERS} ApvlvPopplerPdf.h)
    SET(SOURCES ${SOURCES} ApvlvPopplerPdf.cc)
ELSE ()
    SET(HEADERS ${HEADERS} ApvlvQtPdf.h)
    SET(SOURCES ${SOURCES} ApvlvQtPdf.cc)
ENDIF ()

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

MESSAGE("-- link libraries: ${APVLV_REQ_LIBRARIES}")
ADD_EXECUTABLE(apvlv ${HEADERS} ${SOURCES})
SET_PROPERTY(TARGET apvlv PROPERTY AUTOMOC ON)
TARGET_LINK_LIBRARIES(apvlv ${APVLV_REQ_LIBRARIES})

# for debug
IF (WIN32)
    ADD_CUSTOM_COMMAND(TARGET apvlv POST_BUILD
            COMMAND ${CMAKE_COMMAND}
            "-E" copy_directory
            ${CMAKE_SOURCE_DIR}/share
            ${CMAKE_CURRENT_BINARY_DIR}/share)
    ADD_CUSTOM_COMMAND(TARGET apvlv POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E
            env PATH="${_qt_bin_dir}" "${WINDEPLOYQT_EXECUTABLE}"
            --verbose 0
            --dir "${CMAKE_CURRENT_BINARY_DIR}/Debug"
            $<TARGET_FILE:apvlv>
            COMMENT "Deploying Qt..."
    )
ELSE ()
    ADD_CUSTOM_COMMAND(TARGET apvlv POST_BUILD
            COMMAND ${CMAKE_COMMAND}
            "-E" remove_directory ${CMAKE_BINARY_DIR}/share
            COMMAND ${CMAKE_COMMAND}
            "-E" copy_directory ${CMAKE_SOURCE_DIR}/share
            ${CMAKE_BINARY_DIR}/share)
ENDIF ()

# translate
FIND_PROGRAM(LUPDATE lupdate HINTS "${_qt_bin_dir}")
FIND_PROGRAM(LRELEASE lrelease HINTS "${_qt_bin_dir}")
IF (${LUPDATE} STREQUAL "LUPDATE-NOTFOUND")
    FIND_PROGRAM(LUPDATE lupdate-qt6 HINTS "${_qt_bin_dir}")
    FIND_PROGRAM(LRELEASE lrelease-qt6 HINTS "${_qt_bin_dir}")
ENDIF ()
FILE(GLOB cppfiles *.cc)
ADD_CUSTOM_TARGET(apvlv_lupdate
        COMMAND ${LUPDATE}
        -locations none -target-language zh_CN
        ${cppfiles} -ts ${CMAKE_SOURCE_DIR}/zh_CN.ts)
ADD_CUSTOM_TARGET(apvlv_lrelease
        COMMAND ${LRELEASE}
        ${CMAKE_SOURCE_DIR}/zh_CN.ts
        -qm ${CMAKE_SOURCE_DIR}/share/doc/apvlv/translations/zh_CN.qm)
ADD_DEPENDENCIES(apvlv_lrelease apvlv_lupdate)
ADD_DEPENDENCIES(apvlv apvlv_lrelease)

INSTALL(TARGETS apvlv DESTINATION bin)

IF (WIN32)
    ADD_CUSTOM_COMMAND(TARGET apvlv POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/winqt/"
            COMMAND "${CMAKE_COMMAND}" -E
            env PATH="${_qt_bin_dir}" "${WINDEPLOYQT_EXECUTABLE}"
            --release
            --verbose 0
            --no-compiler-runtime
            --no-opengl-sw
            --no-libraries
            --dir "${CMAKE_CURRENT_BINARY_DIR}/winqt/"
            $<TARGET_FILE:apvlv>
            COMMENT "Deploying Qt..."
    )

    INSTALL(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/winqt/" DESTINATION bin)

    INSTALL(CODE "set(MY_DEPENDENCY_PATHS \"$ENV{PATH}\" \"${_qt_bin_dir}\" )")

    INSTALL(CODE [[
    MESSAGE("-- get runtime dependencies ...")
    LIST(APPEND pre_exclude_regexes "api-ms-.*") # windows API
    LIST(APPEND pre_exclude_regexes "ext-ms-.*") # windows API
    LIST(APPEND pre_exclude_regexes "ieshims.dll") # windows API
    LIST(APPEND pre_exclude_regexes "emclient.dll") # windows API
    LIST(APPEND pre_exclude_regexes "devicelockhelpers.dll") # windows API

    LIST(APPEND post_exclude_regexes ".*WINDOWS[\\/]system32.*")

    FILE(GET_RUNTIME_DEPENDENCIES
      EXECUTABLES $<TARGET_FILE:apvlv>
      RESOLVED_DEPENDENCIES_VAR _r_deps
      UNRESOLVED_DEPENDENCIES_VAR _u_deps
      PRE_EXCLUDE_REGEXES ${pre_exclude_regexes}
      POST_EXCLUDE_REGEXES ${post_exclude_regexes}
      DIRECTORIES ${MY_DEPENDENCY_PATHS}
    )
    LIST(LENGTH _u_deps _u_length)
    IF("${_u_length}" GREATER 0)
      MESSAGE("Unresolved dependencies detected in [${MY_DEPENDENCY_PATHS}]!")
      FOREACH(_no ${_u_deps})
        MESSAGE("Can not find ${_no}")
      ENDFOREACH()
    ENDIF()
    FOREACH(_file ${_r_deps})
      MESSAGE("install " ${_file})
      FILE(INSTALL FILES "${_file}"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
      )
    ENDFOREACH()
    ]])
ENDIF (WIN32)
