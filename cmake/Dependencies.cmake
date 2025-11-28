# Dependency management for apvlv

# Platform-specific dependency handling
if(WIN32)
    # Windows specific Qt6 configuration
    set(Qt6_DIR "C:/Qt/6.7.2/msvc2019_64/lib/cmake/Qt6" CACHE STRING "Qt6config.cmake directory")
    set(Qt6QmlTools_DIR "C:/Qt/6.7.2/msvc2019_64/lib/cmake/Qt6QmlTools" CACHE STRING "Qt6QmlToolsconfig.cmake directory")
    set(Qt6AxContainer_DIR "C:/Qt/6.7.2/msvc2019_64/lib/cmake/Qt6AxContainer" CACHE STRING "Qt6config.cmake directory")

    # Windows specific library paths
    set(QUAZIP_INCLUDE_DIRS "C:/Qt/6.7.2/msvc2019_64/include/QuaZip-Qt6-1.4/quazip;C:/Qt/6.7.2/msvc2019_64/include" CACHE STRING "Quazip include directory")
    set(QUAZIP_LIBRARY_DIRS "C:/Qt/6.7.2/msvc2019_64/lib" CACHE STRING "Quazip lib directory")
    set(QUAZIP_LIBRARIES "C:/Qt/6.7.2/msvc2019_64/lib/quazip1-qt6.lib" CACHE STRING "Quazip library file")
    set(DJVULIBRE_DIR "C:/Program Files (x86)/DjVuLibre" CACHE PATH "DjvuLibre dir")

    # CMake policies for Windows
    cmake_policy(SET CMP0010 NEW)
    cmake_policy(SET CMP0087 NEW)
else()
    # Unix/Linux - use pkg-config
    find_package(PkgConfig QUIET)
    pkg_check_modules(CMARK libcmark REQUIRED)
    pkg_check_modules(QUAZIP quazip1-qt6 REQUIRED)
endif()

# Find Qt6 components
find_package(Qt6 NAMES Qt6 COMPONENTS
    Core Gui Widgets WebEngineWidgets Pdf PdfWidgets Xml PrintSupport
    REQUIRED
)

# Optional dependencies for Unix
if(NOT WIN32)
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
