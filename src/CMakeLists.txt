# Simplified CMakeLists.txt for apvlv src directory

# Include source and engine configurations
include(${CMAKE_CURRENT_SOURCE_DIR}/Sources.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Engines.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/WindowsEngines.cmake)

# Setup include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${QUAZIP_INCLUDE_DIRS}
    ${Qt_INCLUDE_DIRS}
    ${CMARK_INCLUDE_DIRS}
)

# Setup link directories
link_directories(${QUAZIP_LIBRARY_DIRS})

# Initialize required libraries
set(APVLV_REQ_LIBRARIES
    ${QUAZIP_LIBRARIES}
    ${Qt_LIBRARIES}
    ${CMARK_LIBRARIES}
)

# Add engine-specific files and libraries
list(APPEND HEADERS ${APVLV_ENGINE_HEADERS})
list(APPEND SOURCES ${APVLV_ENGINE_SOURCES})
list(APPEND APVLV_REQ_LIBRARIES ${APVLV_ENGINE_LIBRARIES})

# Message about engines
message(STATUS "Building with QtPdf engine")
message(STATUS "Building with Web engine for EPUB/FB2")

# Format target for clang-format
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    # Get all source files
    file(GLOB_RECURSE ALL_CXX_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    )

    # Create format target
    add_custom_target(format
        COMMAND ${CLANG_FORMAT}
        -i
        -style=file
        ${ALL_CXX_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Formatting source code with clang-format"
    )

    message(STATUS "Added 'make format' target for code formatting")
else()
    message(WARNING "clang-format not found, 'make format' target not available")
endif()

# Create executable
add_executable(apvlv ${HEADERS} ${SOURCES})
set_property(TARGET apvlv PROPERTY AUTOMOC ON)
target_link_libraries(apvlv ${APVLV_REQ_LIBRARIES})

# Test executable
add_executable(testNote ApvlvNote.cc ApvlvMarkdown.cc testNote.cc)
set_property(TARGET testNote PROPERTY AUTOMOC ON)
target_link_libraries(testNote ${APVLV_REQ_LIBRARIES})

# Post-build operations
if(WIN32)
    # Windows specific post-build
    add_custom_command(TARGET apvlv POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/share ${CMAKE_CURRENT_BINARY_DIR}/share
    )
else()
    # Unix specific post-build
    add_custom_command(TARGET apvlv POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/share
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/share ${CMAKE_BINARY_DIR}/share
    )
endif()

# Translation setup
find_program(LUPDATE lupdate HINTS "${_qt_bin_dir}")
find_program(LRELEASE lrelease HINTS "${_qt_bin_dir}")
if(LUPDATE STREQUAL "LUPDATE-NOTFOUND")
    find_program(LUPDATE lupdate-qt6 HINTS "${_qt_bin_dir}")
    find_program(LRELEASE lrelease-qt6 HINTS "${_qt_bin_dir}")
endif()

file(GLOB cppfiles *.cc)
add_custom_target(apvlv_lupdate
    COMMAND ${LUPDATE} -locations none -target-language zh_CN
    ${cppfiles} -ts ${CMAKE_SOURCE_DIR}/zh_CN.ts
)
add_custom_target(apvlv_lrelease
    COMMAND ${LRELEASE} ${CMAKE_SOURCE_DIR}/zh_CN.ts
    -qm ${CMAKE_SOURCE_DIR}/share/doc/apvlv/translations/zh_CN.qm
)
add_dependencies(apvlv_lrelease apvlv_lupdate)
add_dependencies(apvlv apvlv_lrelease)

# Installation
install(TARGETS apvlv DESTINATION bin)
