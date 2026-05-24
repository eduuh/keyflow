cmake_minimum_required(VERSION 3.14)

# Include Google Test helpers
include(GoogleTest)

# Collect all test source files
file(GLOB_RECURSE TEST_SOURCES
    "unit/**/*.cpp"
)

# Create test executable
add_executable(keyflow_tests ${TEST_SOURCES})

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

# Set C++17 standard (matching main project)
target_compile_features(keyflow_tests PRIVATE cxx_std_17)

# Copy test data to build directory (handles multi-config generators like MSVC)
add_custom_command(TARGET keyflow_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/tests/fixtures
    $<TARGET_FILE_DIR:keyflow_tests>/fixtures
)

# Register tests with CTest
gtest_discover_tests(keyflow_tests)
