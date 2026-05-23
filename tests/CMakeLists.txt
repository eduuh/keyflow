cmake_minimum_required(VERSION 3.14)

# Include Google Test helpers
include(GoogleTest)

# Collect all test source files
file(GLOB_RECURSE TEST_SOURCES
    "unit/**/*.cpp"
)

# Create test executable (include DebugLog.cpp for gVerboseLogging symbol)
add_executable(keyflow_tests
    ${TEST_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/DebugLog.cpp
)

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
)

# Set C++17 standard (matching main project)
target_compile_features(keyflow_tests PRIVATE cxx_std_17)

# Register tests with CTest
gtest_discover_tests(keyflow_tests)
