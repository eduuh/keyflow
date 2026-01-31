cmake_minimum_required(VERSION 3.15)
project(keyflow VERSION 2.0.0 LANGUAGES CXX)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
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

# Executable
add_executable(keyflow ${SOURCES})

# Link Interception library
target_link_libraries(keyflow PRIVATE interception)

# Output directory
set_target_properties(keyflow PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy interception.dll to output directory
add_custom_command(TARGET keyflow POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/interception.dll"
        "$<TARGET_FILE_DIR:keyflow>/interception.dll"
    COMMENT "Copying interception.dll to output directory"
)

# Copy config.json to output directory
add_custom_command(TARGET keyflow POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/config.json"
        "$<TARGET_FILE_DIR:keyflow>/config.json"
    COMMENT "Copying config.json to output directory"
)

# Installation
install(TARGETS keyflow DESTINATION bin)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/interception.dll" DESTINATION bin)

message(STATUS "keyflow - Configuration complete")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  C++ Standard: C++${CMAKE_CXX_STANDARD}")
