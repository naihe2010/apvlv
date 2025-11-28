# Compiler flags configuration

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Common flags
if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "-Wall -fno-strict-aliasing")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "-D_DEBUG -g ${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "-O2 ${CMAKE_CXX_FLAGS}")
    endif()
else()
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Export compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt definitions
add_definitions(-DQT_MESSAGELOGCONTEXT)
