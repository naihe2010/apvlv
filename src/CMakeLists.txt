# compile options
IF (WIN32)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ELSE ()
    SET(CMAKE_CXX_FLAGS "-std=c++20 -Wall -fno-strict-aliasing")

    IF (${CMAKE_BUILD_TYPE} STREQUAL Debug)
        SET(CMAKE_CXX_FLAGS "-D_DEBUG -g ${CMAKE_CXX_FLAGS}")
    ELSE ()
        SET(CMAKE_CXX_FLAGS "-O2 ${CMAKE_CXX_FLAGS}")
    ENDIF ()
ENDIF ()
MESSAGE("-- BUILD TYPE: ${CMAKE_BUILD_TYPE}")
MESSAGE("-- CXX FLAGS: ${CMAKE_CXX_FLAGS}")

OPTION(APVLV_WITH_MUPDF "If build binary with MuPDF." On)
IF (WIN32)
    OPTION(APVLV_WITH_POPPLER "If build binary with Poppler-Qt6." On)
ELSE ()
    OPTION(APVLV_WITH_POPPLER "If build binary with Poppler-Qt6." Off)
ENDIF ()

IF (WIN32)
    SET(Qt6_DIR "C:/Qt/6.7.2/msvc2019_64/lib/cmake/Qt6" CACHE STRING "Qt6config.cmake directory")
    SET(Qt6QmlTools_DIR "C:/Qt/6.7.2/msvc2019_64/lib/cmake/Qt6QmlTools" CACHE STRING "Qt6QmlToolsconfig.cmake directory")
    SET(Qt6AxContainer_DIR "C:/Qt/6.7.2/msvc2019_64/lib/cmake/Qt6AxContainer" CACHE STRING "Qt6config.cmake directory")
    SET(QUAZIP_INCLUDE_DIRS "C:/Qt/6.7.2/msvc2019_64/include/QuaZip-Qt6-1.4/quazip;C:/Qt/6.7.2/msvc2019_64/include" CACHE STRING "Quazip include directory")
    SET(QUAZIP_LIBRARY_DIRS "C:/Qt/6.7.2/msvc2019_64/lib" CACHE STRING "Quazip lib directory")
    SET(QUAZIP_LIBRARIES "C:/Qt/6.7.2/msvc2019_64/lib/quazip1-qt6.lib" CACHE STRING "Quazip library file")
    SET(DJVULIBRE_DIR "C:/Program Files (x86)/DjVuLibre" CACHE PATH "DjvuLibre dir")
    CMAKE_POLICY(SET CMP0010 NEW)
    CMAKE_POLICY(SET CMP0087 NEW)
ELSE ()
    FIND_PACKAGE(PkgConfig)
    IF (${APVLV_WITH_POPPLER})
        PKG_CHECK_MODULES(POPPLER poppler-qt6)
    ENDIF ()
    PKG_CHECK_MODULES(QUAZIP quazip1-qt6 REQUIRED)
ENDIF ()

FIND_PACKAGE(Qt6 NAMES Qt6 COMPONENTS Core Gui Widgets WebEngineWidgets Pdf PdfWidgets Xml PrintSupport REQUIRED)

SET(Qt_INCLUDE_DIRS
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Gui_INCLUDE_DIRS}
        ${Qt6Widgets_INCLUDE_DIRS}
        ${Qt6WebEngineWidgets_INCLUDE_DIRS}
        ${Qt6Pdf_INCLUDE_DIRS}
        ${Qt6PdfWidgets_INCLUDE_DIRS}
        ${Qt6Xml_INCLUDE_DIRS}
        ${Qt6PrintSupport_INCLUDE_DIRS})
SET(Qt_LIBRARIES Qt6::Core Qt6::Gui Qt6::Widgets Qt6::WebEngineWidgets Qt6::Pdf Qt6::PdfWidgets Qt6::Xml Qt6::PrintSupport)
GET_FILENAME_COMPONENT(_qt_bin_dir "${WINDEPLOYQT_EXECUTABLE}" DIRECTORY)
MESSAGE("-- Qt6 binary directory: ${_qt_bin_dir}")

INCLUDE_DIRECTORIES(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${QUAZIP_INCLUDE_DIRS}
        ${Qt_INCLUDE_DIRS})
LINK_DIRECTORIES(
        ${QUAZIP_LIBRARY_DIRS}
        ${Qt_LIBRARY_DIRS})
SET(APVLV_REQ_LIBRARIES
        ${QUAZIP_LIBRARIES}
        ${Qt_LIBRARIES})

ADD_DEFINITIONS(-DQT_MESSAGELOGCONTEXT)

SET(HEADERS
        ApvlvCmds.h
        ApvlvFrame.h
        ApvlvFile.h
        ApvlvFileIndex.h
        ApvlvFileWidget.h
        ApvlvWidget.h
        ApvlvWeb.h
        ApvlvInfo.h
        ApvlvParams.h
        ApvlvUtil.h
        ApvlvView.h
        ApvlvWindow.h
        ApvlvCompletion.h
        ApvlvContent.h
        ApvlvLab.h
        ApvlvLog.h
        ApvlvSearch.h
        ApvlvSearchDialog.h
        ApvlvDired.h
        ApvlvQueue.h
        ApvlvImageWidget.h
        ApvlvWebViewWidget.h
        ApvlvEditor.h
        file/ApvlvHtm.h
        file/ApvlvImage.h
        file/ApvlvQtPdf.h
        file/ApvlvEpub.h
        file/ApvlvFb2.h
        file/ApvlvTxt.h
)

SET(SOURCES
        ApvlvCmds.cc
        ApvlvFrame.cc
        ApvlvFile.cc
        ApvlvFileIndex.cc
        ApvlvFileWidget.cc
        ApvlvWidget.cc
        ApvlvWeb.cc
        ApvlvInfo.cc
        ApvlvParams.cc
        ApvlvUtil.cc
        ApvlvView.cc
        ApvlvWindow.cc
        ApvlvCompletion.cc
        ApvlvContent.cc
        ApvlvLab.cc
        ApvlvLog.cc
        ApvlvSearch.cc
        ApvlvSearchDialog.cc
        ApvlvDired.cc
        ApvlvImageWidget.cc
        ApvlvWebViewWidget.cc
        ApvlvEditor.cc
        file/ApvlvHtm.cc
        file/ApvlvImage.cc
        file/ApvlvQtPdf.cc
        file/ApvlvEpub.cc
        file/ApvlvFb2.cc
        file/ApvlvTxt.cc
        main.cc
)

MESSAGE("-- will build Web as epub engine")
MESSAGE("-- will build Web as fb2 engine")
MESSAGE("-- will build QtPdf as pdf engine")

IF (${APVLV_WITH_MUPDF})
    MESSAGE("-- will build MuPDF as pdf engine")
    SET(HEADERS ${HEADERS} file/ApvlvMuPdf.h)
    SET(SOURCES ${SOURCES} file/ApvlvMuPdf.cc)
    IF (WIN32)
        SET(APVLV_REQ_LIBRARIES mupdf.lib ${APVLV_REQ_LIBRARIES})
    ELSE ()
        SET(APVLV_REQ_LIBRARIES -lmupdf ${APVLV_REQ_LIBRARIES})
    ENDIF ()
ENDIF ()

IF (${APVLV_WITH_POPPLER})
    MESSAGE("-- will build Poppler as pdf engine")
    SET(HEADERS ${HEADERS} file/ApvlvPopplerPdf.h)
    SET(SOURCES ${SOURCES} file/ApvlvPopplerPdf.cc)
    INCLUDE_DIRECTORIES(${POPPLER_INCLUDE_DIRS})
    LINK_DIRECTORIES(${POPPLER_LIBRARY_DIRS})
    SET(APVLV_REQ_LIBRARIES ${POPPLER_LIBRARIES} ${APVLV_REQ_LIBRARIES})
ENDIF ()

OPTION(APVLV_WITH_DJVU "If build apvlv with djvu format support." ON)
IF (APVLV_WITH_DJVU)
    MESSAGE("-- will build djvulibre as djvu engine")
    IF (WIN32)
        INCLUDE_DIRECTORIES(${DJVULIBRE_DIR}/include)
        LINK_DIRECTORIES(${DJVULIBRE_DIR})
        SET(APVLV_REQ_LIBRARIES libdjvulibre.lib
                ${APVLV_REQ_LIBRARIES})
    ELSE (WIN32)
        SET(APVLV_REQ_LIBRARIES -ldjvulibre
                ${APVLV_REQ_LIBRARIES})
    ENDIF (WIN32)

    SET(HEADERS ${HEADERS} file/ApvlvDjvu.h)
    SET(SOURCES ${SOURCES} file/ApvlvDjvu.cc)
ENDIF (APVLV_WITH_DJVU)

OPTION(APVLV_WITH_OFFICE "If build apvlv with office format support." ON)
IF (APVLV_WITH_OFFICE)
    IF (WIN32)
        MESSAGE("-- will build MSOffice as office file engine")
        FIND_PACKAGE(Qt6AxContainer REQUIRED)
        INCLUDE_DIRECTORIES(${Qt6AxContainer_INCLUDE_DIRES})
        SET(APVLV_REQ_LIBRARIES ${Qt6AxContainer_LIBRARIES} Shlwapi.lib
                ${APVLV_REQ_LIBRARIES})
        SET(HEADERS ${HEADERS} file/ApvlvAxOffice.h)
        SET(SOURCES ${SOURCES} file/ApvlvAxOffice.cc)
    ELSE (WIN32)
        MESSAGE("-- will build libreOffice as office file engine")
        SET(APVLV_REQ_LIBRARIES -llibreofficekitgtk
                ${APVLV_REQ_LIBRARIES})
        ADD_DEFINITIONS(-DLIBO_INTERNAL_ONLY=1)
        SET(HEADERS ${HEADERS} file/ApvlvLibreOffice.h)
        SET(SOURCES ${SOURCES} file/ApvlvLibreOffice.cc)
    ENDIF (WIN32)
ENDIF (APVLV_WITH_OFFICE)

OPTION(APVLV_WITH_OCR "If build apvlv with OCR support." ON)
IF (${APVLV_WITH_OCR})
    ADD_DEFINITIONS(-DAPVLV_WITH_OCR)
    SET(HEADERS ${HEADERS} ApvlvOCR.h)
    SET(SOURCES ${SOURCES} ApvlvOCR.cc)

    IF (WIN32)
    ELSE ()
        MESSAGE("-- will build opencv and tesseract as OCR library")
        PKG_CHECK_MODULES(TESSERACT tesseract opencv)
        INCLUDE_DIRECTORIES(${TESSERACT_INCLUDE_DIRS})
        LINK_DIRECTORIES(${TESSERACT_LIBRARY_DIRS})
        SET(APVLV_REQ_LIBRARIES ${TESSERACT_LIBRARIES}
                ${APVLV_REQ_LIBRARIES})
    ENDIF ()
ENDIF ()

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
