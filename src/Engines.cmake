# Document engine configuration

# Initialize engine-specific variables
set(APVLV_ENGINE_HEADERS "")
set(APVLV_ENGINE_SOURCES "")
set(APVLV_ENGINE_LIBRARIES "")

# MuPDF engine
if(APVLV_WITH_MUPDF)
    message(STATUS "Enable MuPDF engine")
    list(APPEND APVLV_ENGINE_HEADERS file/ApvlvMuPdf.h)
    list(APPEND APVLV_ENGINE_SOURCES file/ApvlvMuPdf.cc)

    if(WIN32)
        list(APPEND APVLV_ENGINE_LIBRARIES mupdf.lib)
    else()
        if(MUPDF_FOUND)
            list(APPEND APVLV_ENGINE_LIBRARIES ${MUPDF_STATIC_LIBRARIES} -lharfbuzz)
        else()
            list(APPEND APVLV_ENGINE_LIBRARIES -lmupdf)
        endif()
    endif()
endif()

# Poppler engine (Windows only)
if(WIN32 AND APVLV_WITH_POPPLER)
    message(STATUS "Enable Poppler engine")
    list(APPEND APVLV_ENGINE_HEADERS file/ApvlvPopplerPdf.h)
    list(APPEND APVLV_ENGINE_SOURCES file/ApvlvPopplerPdf.cc)
elseif(NOT WIN32 AND APVLV_WITH_POPPLER AND POPPLER_FOUND)
    message(STATUS "Enable Poppler engine")
    list(APPEND APVLV_ENGINE_HEADERS file/ApvlvPopplerPdf.h)
    list(APPEND APVLV_ENGINE_SOURCES file/ApvlvPopplerPdf.cc)
    list(APPEND APVLV_ENGINE_LIBRARIES ${POPPLER_LIBRARIES})
endif()

# DjVu support
if(APVLV_WITH_DJVU)
    message(STATUS "Enable DjVu support")
    list(APPEND APVLV_ENGINE_HEADERS file/ApvlvDjvu.h)
    list(APPEND APVLV_ENGINE_SOURCES file/ApvlvDjvu.cc)

    if(WIN32)
        include_directories(${DJVULIBRE_DIR}/include)
        link_directories(${DJVULIBRE_DIR})
        list(APPEND APVLV_ENGINE_LIBRARIES libdjvulibre.lib)
    else()
        list(APPEND APVLV_ENGINE_LIBRARIES -ldjvulibre)
    endif()
endif()

# OCR support
if(APVLV_WITH_OCR)
    message(STATUS "Enable OCR support")
    add_definitions(-DAPVLV_WITH_OCR)
    list(APPEND APVLV_ENGINE_HEADERS ApvlvOCR.h)
    list(APPEND APVLV_ENGINE_SOURCES ApvlvOCR.cc)

    if(WIN32)
        # Windows OCR may use different libraries
        # Add Windows-specific OCR configuration here if needed
    else()
        if(TESSERACT_FOUND)
            list(APPEND APVLV_ENGINE_LIBRARIES ${TESSERACT_LIBRARIES})
        endif()
    endif()
endif()
