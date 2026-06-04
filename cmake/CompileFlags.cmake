# Compiler flags configuration

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Common flags
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -fno-strict-aliasing)
    add_compile_definitions($<$<CONFIG:Debug>:_DEBUG>)
endif()

# Export compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt definitions
add_definitions(-DQT_MESSAGELOGCONTEXT)
