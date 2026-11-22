cmake_minimum_required(VERSION 3.14)

# Include Google Test helpers
include(GoogleTest)

# Collect all test source files
file(GLOB_RECURSE TEST_SOURCES
    "unit/**/*.cpp"
)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(FILTER TEST_SOURCES EXCLUDE REGEX "/platform/linux/")
elseif(NOT ENABLE_LINUX_BACKEND)
    list(FILTER TEST_SOURCES EXCLUDE REGEX "/test_linux_(hardware|keycodes)\\.cpp$")
endif()

# Create test executable (include DebugLog.cpp for gVerboseLogging symbol)
add_executable(keyflow_tests
    ${TEST_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/app/DebugLog.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(keyflow_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src/platform/linux/LinuxPlatformInit.cpp
        ${CMAKE_SOURCE_DIR}/src/platform/linux/LinuxSingleInstanceLock.cpp
    )
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND ENABLE_LINUX_BACKEND)
    target_sources(keyflow_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src/platform/linux/LinuxHardwareIO.cpp
    )
    target_link_libraries(keyflow_tests PRIVATE PkgConfig::LIBEVDEV)
endif()

# Link against Google Test and keyflow headers
target_link_libraries(keyflow_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)

# Include keyflow source directories
target_include_directories(keyflow_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

# Expose the live keyboard config path so integration tests can load it
# directly. The config is the test spec: edit src/config.json and the
# remapping test surface automatically tracks.
target_compile_definitions(keyflow_tests PRIVATE
    KEYFLOW_CONFIG_PATH="${CMAKE_SOURCE_DIR}/src/config.json"
    KEYFLOW_DESIGNER_CONFIG_PATH="${CMAKE_SOURCE_DIR}/designer/public/eduuh-dh-config.json"
)

# Set C++17 standard (matching main project)
target_compile_features(keyflow_tests PRIVATE cxx_std_17)

# Register tests with CTest
gtest_discover_tests(keyflow_tests)

option(ENABLE_LINUX_DEVICE_TESTS "Enable authorized synthetic evdev/uinput tests" OFF)
if(ENABLE_LINUX_DEVICE_TESTS)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT ENABLE_LINUX_BACKEND)
        message(FATAL_ERROR "Linux device tests require the Linux runtime backend")
    endif()
    find_package(Threads REQUIRED)
    add_executable(keyflow_linux_device_tests
        device/test_linux_device.cpp
        ${CMAKE_SOURCE_DIR}/src/app/DebugLog.cpp
        ${CMAKE_SOURCE_DIR}/src/platform/linux/LinuxHardwareIO.cpp
    )
    target_include_directories(keyflow_linux_device_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests
    )
    target_compile_definitions(keyflow_linux_device_tests PRIVATE
        KEYFLOW_CONFIG_PATH="${CMAKE_SOURCE_DIR}/src/config.json"
        KEYFLOW_DESIGNER_CONFIG_PATH="${CMAKE_SOURCE_DIR}/designer/public/eduuh-dh-config.json"
    )
    target_link_libraries(keyflow_linux_device_tests PRIVATE
        GTest::gtest_main PkgConfig::LIBEVDEV Threads::Threads
    )
    gtest_discover_tests(keyflow_linux_device_tests
        TEST_PREFIX "LinuxDevice/"
        PROPERTIES LABELS "linux-device" TIMEOUT 15 RESOURCE_LOCK keyflow-kernel-io
    )
endif()
