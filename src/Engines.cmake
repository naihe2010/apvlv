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
    list(APPEND APVLV_ENGINE_LIBRARIES ${MUPDF_LIBRARIES})
  elseif(MUPDF_FOUND)
    list(APPEND APVLV_ENGINE_LIBRARIES ${MUPDF_STATIC_LIBRARIES} -lharfbuzz)
  else()
    list(APPEND APVLV_ENGINE_LIBRARIES -lmupdf)
  endif()
endif()

# Poppler engine
if(APVLV_WITH_POPPLER)
  message(STATUS "Enable Poppler engine")
  list(APPEND APVLV_ENGINE_HEADERS file/ApvlvPopplerPdf.h)
  list(APPEND APVLV_ENGINE_SOURCES file/ApvlvPopplerPdf.cc)
  if(NOT WIN32)
    include_directories(${POPPLER_INCLUDE_DIRS})
  endif()
  list(APPEND APVLV_ENGINE_LIBRARIES ${POPPLER_LIBRARIES})
endif()

# DjVu support
if(APVLV_WITH_DJVU)
  message(STATUS "Enable DjVu support")
  list(APPEND APVLV_ENGINE_HEADERS file/ApvlvDjvu.h)
  list(APPEND APVLV_ENGINE_SOURCES file/ApvlvDjvu.cc)
  if(WIN32)
    list(APPEND APVLV_ENGINE_LIBRARIES ${DJVU_LIBRARIES})
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
  list(APPEND APVLV_ENGINE_LIBRARIES ${TESSERACT_LIBRARIES})
endif()

# Office support
if(APVLV_WITH_OFFICE)
  if(WIN32)
    message(STATUS "Enable MSOffice as office file engine")
    find_package(Qt6 REQUIRED COMPONENTS AxContainer)
    list(APPEND APVLV_ENGINE_LIBRARIES Qt6::AxContainer Shlwapi.lib)
    list(APPEND APVLV_ENGINE_HEADERS file/ApvlvAxOffice.h)
    list(APPEND APVLV_ENGINE_SOURCES file/ApvlvAxOffice.cc)
  else()
    message(STATUS "Enable libreOffice as office file engine")
    add_definitions(-DLIBO_INTERNAL_ONLY=1)
    list(APPEND APVLV_ENGINE_LIBRARIES -llibreofficekitgtk)
    list(APPEND APVLV_ENGINE_HEADERS file/ApvlvLibreOffice.h)
    list(APPEND APVLV_ENGINE_SOURCES file/ApvlvLibreOffice.cc)
  endif()
endif()
