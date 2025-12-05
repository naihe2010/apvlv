# Dependency management for apvlv

# Platform-specific dependency handling
if(WIN32)
    # Windows - use vcpkg via find_package (vcpkg provides CMake config files)
    # Required dependencies
    find_package(cmark REQUIRED)
    find_package(quazip REQUIRED)
    find_package(djvulibre REQUIRED)
    
    # Setup variables for compatibility with Unix code
    if(TARGET cmark::cmark)
        set(CMARK_FOUND TRUE)
        get_target_property(CMARK_INCLUDE_DIRS cmark::cmark INTERFACE_INCLUDE_DIRECTORIES)
        if(NOT CMARK_INCLUDE_DIRS)
            get_target_property(CMARK_INCLUDE_DIRS cmark::cmark INCLUDE_DIRECTORIES)
        endif()
        set(CMARK_LIBRARIES cmark::cmark)
    elseif(cmark_FOUND)
        set(CMARK_FOUND TRUE)
        if(cmark_INCLUDE_DIR)
            set(CMARK_INCLUDE_DIRS ${cmark_INCLUDE_DIR})
        endif()
        if(cmark_LIBRARY)
            set(CMARK_LIBRARIES ${cmark_LIBRARY})
        endif()
    endif()
    
    if(TARGET quazip::quazip)
        set(QUAZIP_FOUND TRUE)
        get_target_property(QUAZIP_INCLUDE_DIRS quazip::quazip INTERFACE_INCLUDE_DIRECTORIES)
        if(NOT QUAZIP_INCLUDE_DIRS)
            get_target_property(QUAZIP_INCLUDE_DIRS quazip::quazip INCLUDE_DIRECTORIES)
        endif()
        set(QUAZIP_LIBRARIES quazip::quazip)
    elseif(quazip_FOUND)
        set(QUAZIP_FOUND TRUE)
        if(quazip_INCLUDE_DIR)
            set(QUAZIP_INCLUDE_DIRS ${quazip_INCLUDE_DIR})
        endif()
        if(quazip_LIBRARY)
            set(QUAZIP_LIBRARIES ${quazip_LIBRARY})
        endif()
    endif()
    
    # DjVu setup for Windows
    if(TARGET djvulibre::djvulibre)
        set(DJVULIBRE_FOUND TRUE)
        get_target_property(DJVULIBRE_INCLUDE_DIR djvulibre::djvulibre INTERFACE_INCLUDE_DIRECTORIES)
        if(DJVULIBRE_INCLUDE_DIR)
            get_filename_component(DJVULIBRE_DIR ${DJVULIBRE_INCLUDE_DIR} DIRECTORY)
        endif()
    elseif(djvulibre_FOUND)
        set(DJVULIBRE_FOUND TRUE)
        if(djvulibre_INCLUDE_DIR)
            set(DJVULIBRE_INCLUDE_DIR ${djvulibre_INCLUDE_DIR})
            get_filename_component(DJVULIBRE_DIR ${djvulibre_INCLUDE_DIR} DIRECTORY)
        endif()
        if(djvulibre_LIBRARY)
            get_filename_component(DJVULIBRE_LIB_DIR ${djvulibre_LIBRARY} DIRECTORY)
            if(NOT DJVULIBRE_DIR)
                set(DJVULIBRE_DIR ${DJVULIBRE_LIB_DIR})
            endif()
        endif()
    endif()
    
    # Optional dependencies for Windows
    if(APVLV_WITH_POPPLER)
        find_package(Poppler QUIET)
        if(Poppler_FOUND)
            set(POPPLER_FOUND TRUE)
            if(TARGET Poppler::poppler)
                set(POPPLER_LIBRARIES Poppler::poppler)
            elseif(poppler_LIBRARIES)
                set(POPPLER_LIBRARIES ${poppler_LIBRARIES})
            endif()
        endif()
    endif()
    
    if(APVLV_WITH_MUPDF)
        find_package(mupdf QUIET)
        if(mupdf_FOUND)
            set(MUPDF_FOUND TRUE)
            if(TARGET mupdf::mupdf)
                set(MUPDF_STATIC_LIBRARIES mupdf::mupdf)
            elseif(mupdf_LIBRARIES)
                set(MUPDF_STATIC_LIBRARIES ${mupdf_LIBRARIES})
            endif()
        endif()
    endif()
    
    if(APVLV_WITH_OCR)
        find_package(tesseract QUIET)
        if(tesseract_FOUND)
            set(TESSERACT_FOUND TRUE)
            if(TARGET tesseract::tesseract)
                set(TESSERACT_LIBRARIES tesseract::tesseract)
            elseif(tesseract_LIBRARIES)
                set(TESSERACT_LIBRARIES ${tesseract_LIBRARIES})
            endif()
        endif()
    endif()
else()
    # Unix/Linux - use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CMARK libcmark REQUIRED)
    pkg_check_modules(QUAZIP quazip1-qt6 REQUIRED)
    
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
