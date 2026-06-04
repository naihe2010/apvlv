# Dependency management for apvlv

# Platform-specific dependency handling
if(WIN32)
    # Windows - dependencies provided by vcpkg (CMake config packages)
    find_package(cmark CONFIG REQUIRED)
    set(CMARK_LIBRARIES cmark::cmark)

    find_package(QuaZip-Qt6 CONFIG REQUIRED)
    set(QUAZIP_LIBRARIES QuaZip::QuaZip)

    if(APVLV_WITH_DJVU)
        find_package(djvulibre REQUIRED)
        set(DJVU_LIBRARIES djvulibre::djvulibre)
    endif()

    if(APVLV_WITH_POPPLER)
        find_package(Poppler CONFIG REQUIRED)
        set(POPPLER_LIBRARIES Poppler::poppler)
    endif()

    if(APVLV_WITH_MUPDF)
        find_package(unofficial-libmupdf CONFIG REQUIRED)
        set(MUPDF_LIBRARIES unofficial::libmupdf::libmupdf)
    endif()

    if(APVLV_WITH_OCR)
        find_package(Tesseract CONFIG REQUIRED)
        set(TESSERACT_LIBRARIES Tesseract::libtesseract)
    endif()
else()
    # Unix/Linux - use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CMARK libcmark REQUIRED)
    pkg_check_modules(QUAZIP quazip1-qt6)
    if(NOT QUAZIP_FOUND)
        find_package(QuaZip-Qt6 CONFIG REQUIRED)
        set(QUAZIP_LIBRARIES QuaZip::QuaZip)
    endif()
    
    # Optional dependencies for Unix
    if(APVLV_WITH_POPPLER)
        pkg_check_modules(POPPLER poppler-qt6)
    endif()

    if(APVLV_WITH_MUPDF)
        pkg_check_modules(MUPDF mupdf)
    endif()

    if(APVLV_WITH_OCR)
        pkg_check_modules(TESSERACT tesseract)
    endif()
endif()

# Find Qt6 components
find_package(Qt6 NAMES Qt6 COMPONENTS
    Core Gui Widgets WebEngineWidgets Pdf PdfWidgets Xml PrintSupport
    REQUIRED
)

find_package(Qt6 NAMES Qt6 OPTIONAL_COMPONENTS Core5Compat)

# Setup Qt variables
set(Qt_INCLUDE_DIRS
    ${Qt6Core_INCLUDE_DIRS}
    ${Qt6Gui_INCLUDE_DIRS}
    ${Qt6Widgets_INCLUDE_DIRS}
    ${Qt6WebEngineWidgets_INCLUDE_DIRS}
    ${Qt6Pdf_INCLUDE_DIRS}
    ${Qt6PdfWidgets_INCLUDE_DIRS}
    ${Qt6Xml_INCLUDE_DIRS}
    ${Qt6PrintSupport_INCLUDE_DIRS}
)

set(Qt_LIBRARIES
    Qt6::Core Qt6::Gui Qt6::Widgets
    Qt6::WebEngineWidgets Qt6::Pdf Qt6::PdfWidgets
    Qt6::Xml Qt6::PrintSupport
)

if(Qt6Core5Compat_FOUND)
    list(APPEND Qt_INCLUDE_DIRS ${Qt6Core5Compat_INCLUDE_DIRS})
    list(APPEND Qt_LIBRARIES Qt6::Core5Compat)
endif()
