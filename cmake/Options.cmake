if(NOT CMAKE_CONFIGURATION_TYPES)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)
endif()

# Build options for apvlv
option(APVLV_WITH_MUPDF "Enable MuPDF PDF engine" ON)
option(APVLV_WITH_POPPLER "Enable Poppler PDF engine" ON)
option(APVLV_WITH_DJVU "Enable DjVu support" ON)
option(APVLV_WITH_OFFICE "Enable Office document support" ON)
option(APVLV_WITH_OCR "Enable OCR support" ON)
