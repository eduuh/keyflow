cmake_minimum_required(VERSION 3.15)
project(keyflow VERSION 2.0.0 LANGUAGES CXX)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Define DEBUG for Debug builds (compile out in Release)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_BUILD)
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX)
    add_compile_options(/O2)  # Release optimization
else()
    add_compile_options(-Wall -Wextra -Werror)
    add_compile_options(-O3)  # Release optimization
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform   # For interception.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/external   # For nlohmann/json
)

# Library directories (for interception.lib)
link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
)

# Source files
set(SOURCES
    src/main.cpp
    src/hardware/HardwareIO.cpp
)

# Executable (always console subsystem, hide window at runtime for Release)
add_executable(keyflow ${SOURCES})

# Link Interception library
target_link_libraries(keyflow PRIVATE interception)

# Output directory - organize by build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/debug/bin)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/release/bin)
else()
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)
endif()

set_target_properties(keyflow PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
)

# Copy interception.dll to output directory
add_custom_command(TARGET keyflow POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/interception.dll"
        "${OUTPUT_DIR}/interception.dll"
    COMMENT "Copying interception.dll to ${CMAKE_BUILD_TYPE} output directory"
)

# Copy config.json to output directory
add_custom_command(TARGET keyflow POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/config.json"
        "${OUTPUT_DIR}/config.json"
    COMMENT "Copying config.json to ${CMAKE_BUILD_TYPE} output directory"
)

# Copy keyboard-icon.ico to output directory
add_custom_command(TARGET keyflow POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/keyboard-icon.ico"
        "${OUTPUT_DIR}/keyboard-icon.ico"
    COMMENT "Copying keyboard-icon.ico to ${CMAKE_BUILD_TYPE} output directory"
)

# Installation
install(TARGETS keyflow DESTINATION bin)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/interception.dll" DESTINATION bin)

message(STATUS "keyflow - Configuration complete")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  C++ Standard: C++${CMAKE_CXX_STANDARD}")
