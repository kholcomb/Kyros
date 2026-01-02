# Tests for Kyros
# Using Google Test framework

# Test utilities library (shared test helpers)
add_library(test_utils INTERFACE)
target_include_directories(test_utils INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)
target_link_libraries(test_utils INTERFACE
    kyros-lib
    GTest::gtest
    GTest::gmock
)

# Helper function to add a test executable
function(add_kyros_test TEST_NAME)
    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})

    add_executable(${TEST_NAME} ${ARG_SOURCES})
    target_link_libraries(${TEST_NAME} PRIVATE
        test_utils
        GTest::gtest_main
        GTest::gmock
    )

    # Discover tests automatically
    gtest_discover_tests(${TEST_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTIES
            LABELS "unit"
    )
endfunction()

# Unit Tests
add_kyros_test(test_interrogator
    SOURCES unit/test_scanner.cpp
)

# Detection Engine Tests
add_kyros_test(test_detection_engines
    SOURCES unit/test_detection_engines.cpp
)

# Platform Adapter Tests
add_kyros_test(test_platform_adapters
    SOURCES unit/test_platform_adapters.cpp
)

# Rulepack Tests
add_kyros_test(test_rulepack
    SOURCES unit/test_rulepack.cpp
)

# Evidence Tests
add_kyros_test(test_evidence
    SOURCES unit/test_evidence.cpp
)

# Integration Tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration)
    add_executable(integration_tests
        integration/test_end_to_end.cpp
    )
    target_link_libraries(integration_tests PRIVATE
        test_utils
        GTest::gtest_main
    )
    gtest_discover_tests(integration_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTIES
            LABELS "integration"
    )
endif()

# Code Coverage Support
if(CMAKE_BUILD_TYPE MATCHES "Coverage")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(kyros-lib PUBLIC
            --coverage
            -fprofile-arcs
            -ftest-coverage
        )
        target_link_options(kyros-lib PUBLIC --coverage)
    endif()
endif()
