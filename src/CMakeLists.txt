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
