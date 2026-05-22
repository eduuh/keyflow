cmake_minimum_required(VERSION 3.15)
project(keyflow VERSION 2.0.2 LANGUAGES CXX)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
    add_compile_options(/W4 /WX /MP)  # /MP enables multi-processor compilation
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

# Include directories (platform-agnostic, available to all targets)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/external   # For nlohmann/json
)

# Platform detection
set(KEYFLOW_HAS_PLATFORM_BACKEND FALSE)

if(WIN32)
    set(KEYFLOW_HAS_PLATFORM_BACKEND TRUE)
    message(STATUS "  Platform: Windows")
elseif(APPLE)
    message(STATUS "  Platform: macOS (no runtime backend yet - tests only)")
elseif(UNIX)
    message(STATUS "  Platform: Linux (no runtime backend yet - tests only)")
endif()

# === Keyflow executable (Windows only, requires platform backend) ===
if(KEYFLOW_HAS_PLATFORM_BACKEND)
    if(WIN32)
        include_directories(
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows   # For interception.h
        )
        link_directories(
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows    # For interception.lib
        )
        set(PLATFORM_SOURCES
            src/platform/windows/WindowsHardwareIO.cpp
        )
    endif()

    set(SOURCES
        src/main.cpp
        src/DebugLog.cpp
        ${PLATFORM_SOURCES}
    )

    add_executable(keyflow ${SOURCES})

    if(WIN32)
        target_link_libraries(keyflow PRIVATE interception)
    endif()

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

    # Platform-specific post-build steps
    if(WIN32)
        add_custom_command(TARGET keyflow POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows/interception.dll"
                "$<TARGET_FILE_DIR:keyflow>/interception.dll"
            COMMENT "Copying interception.dll"
        )

        add_custom_command(TARGET keyflow POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/src/keyboard-icon.ico"
                "$<TARGET_FILE_DIR:keyflow>/keyboard-icon.ico"
            COMMENT "Copying keyboard-icon.ico"
        )
    endif()

    add_custom_command(TARGET keyflow POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/src/config.json"
            "$<TARGET_FILE_DIR:keyflow>/config.json"
        COMMENT "Copying config.json"
    )

    # Installation
    install(TARGETS keyflow DESTINATION bin)
    if(WIN32)
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows/interception.dll" DESTINATION bin)
    endif()
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/config.schema.json" DESTINATION bin)
endif()

# === Testing (cross-platform - unit tests work on all platforms) ===
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(ENABLE_TESTS "Enable testing" ON)
else()
    option(ENABLE_TESTS "Enable testing (use -DENABLE_TESTS=ON to enable)" OFF)
endif()

if(ENABLE_TESTS)
    enable_testing()

    # === Google Test Setup ===
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
        FIND_PACKAGE_ARGS NAMES GTest
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # Disable -Werror for Google Test targets
    if(NOT MSVC)
        target_compile_options(gtest PRIVATE -Wno-error=undef)
        target_compile_options(gtest_main PRIVATE -Wno-error=undef)
        target_compile_options(gmock PRIVATE -Wno-error=undef)
        target_compile_options(gmock_main PRIVATE -Wno-error=undef)
    endif()

    # Unit tests (platform-agnostic, always available)
    add_subdirectory(tests)

    # Config validation tests (require the keyflow executable, Windows only)
    if(KEYFLOW_HAS_PLATFORM_BACKEND)
        function(add_config_test test_name config_file should_pass)
            add_test(
                NAME ${test_name}
                COMMAND ${CMAKE_COMMAND}
                    -DKEYFLOW_EXE=$<TARGET_FILE:keyflow>
                    -DCONFIG_FILE=${CMAKE_SOURCE_DIR}/${config_file}
                    -DSHOULD_PASS=${should_pass}
                    -P ${CMAKE_SOURCE_DIR}/cmake/RunConfigTest.cmake
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            )
            set_tests_properties(${test_name} PROPERTIES TIMEOUT 5)
        endfunction()

        # Valid configs
        add_config_test(test_valid_minimal "test/configs/valid/minimal.json" TRUE)
        add_config_test(test_valid_full_remapping "test/configs/valid/full_remapping.json" TRUE)
        add_config_test(test_valid_with_layers "test/configs/valid/with_layers.json" TRUE)
        add_config_test(test_valid_with_combos "test/configs/valid/with_combos.json" TRUE)
        add_config_test(test_valid_complex "test/configs/valid/complex.json" TRUE)

        # Invalid configs
        add_config_test(test_invalid_missing_version "test/configs/invalid/missing_version.json" FALSE)
        add_config_test(test_invalid_unknown_key "test/configs/invalid/unknown_key.json" FALSE)
        add_config_test(test_invalid_empty_layer "test/configs/invalid/empty_layer.json" FALSE)
        add_config_test(test_invalid_malformed "test/configs/invalid/malformed.json" FALSE)
        add_config_test(test_invalid_trigger "test/configs/invalid/invalid_trigger.json" FALSE)

        # Example configs
        add_config_test(test_example_minimal "examples/minimal.json" TRUE)
        add_config_test(test_example_colemak "examples/colemak-dh.json" TRUE)
        add_config_test(test_example_vim "examples/vim-layer.json" TRUE)
        add_config_test(test_example_symbols "examples/programmer-symbols.json" TRUE)
        add_config_test(test_example_numpad "examples/numpad-layer.json" TRUE)
    endif()

    message(STATUS "  Tests: Enabled")
else()
    message(STATUS "  Tests: Disabled (use -DENABLE_TESTS=ON to enable)")
endif()

message(STATUS "keyflow - Configuration complete")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  C++ Standard: C++${CMAKE_CXX_STANDARD}")
