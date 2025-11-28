# Build options for apvlv
option(APVLV_WITH_MUPDF "Enable MuPDF PDF engine" ON)
option(APVLV_WITH_POPPLER "Enable Poppler PDF engine" OFF)
option(APVLV_WITH_DJVU "Enable DjVu support" ON)
option(APVLV_WITH_OFFICE "Enable Office document support" ON)
option(APVLV_WITH_OCR "Enable OCR support" ON)

# Platform-specific defaults
if(WIN32)
    set(APVLV_WITH_POPPLER ON CACHE BOOL "Enable Poppler PDF engine" FORCE)
endif()
