cmake_minimum_required(VERSION 3.15)
project(Kyros VERSION 2.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_DAEMON "Build daemon service mode" ON)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(ENABLE_CONTAINERS "Enable Docker/Kubernetes support" ON)
option(ENABLE_MANAGEMENT_SERVER "Enable management server client" ON)

# Platform detection
if(UNIX AND NOT APPLE)
    set(PLATFORM_LINUX TRUE)
    add_compile_definitions(PLATFORM_LINUX)
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
    add_compile_definitions(PLATFORM_MACOS)
elseif(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    add_compile_definitions(PLATFORM_WINDOWS)
    add_compile_definitions(_WIN32_WINNT=0x0601)  # Windows 7+
endif()

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

# Set binary name
set(KYROS_BINARY_NAME "kyros")

# Find dependencies
find_package(nlohmann_json 3.11.0 QUIET)

# Optional dependencies
if(BUILD_DAEMON)
    find_package(SQLite3 REQUIRED)
endif()

if(ENABLE_MANAGEMENT_SERVER)
    find_package(gRPC)
endif()

# Third-party libraries
add_subdirectory(third_party)

# Generate version header
configure_file(
    ${CMAKE_SOURCE_DIR}/include/kyros/version.hpp.in
    ${CMAKE_BINARY_DIR}/include/kyros/version.hpp
    @ONLY
)

# Main library
add_subdirectory(src)

# Executable
add_executable(${KYROS_BINARY_NAME}
    src/main.cpp
)

target_link_libraries(${KYROS_BINARY_NAME} PRIVATE
    kyros-lib
    CLI11::CLI11
)

# Daemon mode
if(BUILD_DAEMON)
    target_compile_definitions(${KYROS_BINARY_NAME} PRIVATE ENABLE_DAEMON)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(BUILD_EXAMPLES)
    if(EXISTS ${CMAKE_SOURCE_DIR}/examples)
        add_subdirectory(examples)
    else()
        message(STATUS "Examples directory not found, skipping")
    endif()
endif()

# Install targets
install(TARGETS ${KYROS_BINARY_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/kyros
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

# Platform-specific installation
if(PLATFORM_LINUX)
    # Install systemd service file
    install(FILES packaging/linux/kyros.service
        DESTINATION /etc/systemd/system
        OPTIONAL
    )
    # Install default config
    install(FILES config/daemon.conf.example
        DESTINATION /etc/kyros
        RENAME daemon.conf
        OPTIONAL
    )
elseif(PLATFORM_MACOS)
    # Install launchd plist
    install(FILES packaging/macos/com.kyros.daemon.plist
        DESTINATION /Library/LaunchDaemons
        OPTIONAL
    )
endif()

# CPack configuration
set(CPACK_PACKAGE_NAME "kyros")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Kyros - MCP Server Detection Engine")
set(CPACK_PACKAGE_VENDOR "Kyros Project")

if(PLATFORM_LINUX)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsqlite3-0")
elseif(PLATFORM_MACOS)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
elseif(PLATFORM_WINDOWS)
    set(CPACK_GENERATOR "NSIS;ZIP")
endif()

include(CPack)

# Code Coverage Support
if(CMAKE_BUILD_TYPE MATCHES "Coverage")
    message(STATUS "Code coverage build enabled")

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Add coverage flags to all targets
        add_compile_options(
            --coverage
            -fprofile-arcs
            -ftest-coverage
            -O0
            -g
        )
        add_link_options(--coverage)

        # Add coverage target
        if(BUILD_TESTS)
            add_custom_target(coverage
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
                COMMAND lcov --directory . --zerocounters
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                COMMAND lcov --directory . --capture --output-file ${CMAKE_BINARY_DIR}/coverage/coverage.info
                COMMAND lcov --remove ${CMAKE_BINARY_DIR}/coverage/coverage.info
                    '/usr/*'
                    '*/third_party/*'
                    '*/tests/*'
                    --output-file ${CMAKE_BINARY_DIR}/coverage/coverage_filtered.info
                COMMAND genhtml ${CMAKE_BINARY_DIR}/coverage/coverage_filtered.info
                    --output-directory ${CMAKE_BINARY_DIR}/coverage/html
                COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated in ${CMAKE_BINARY_DIR}/coverage/html/index.html"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating code coverage report"
            )
        endif()
    else()
        message(WARNING "Code coverage requires GCC or Clang compiler")
    endif()
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "Kyros Configuration Summary")
message(STATUS "===========================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "")
message(STATUS "Options:")
message(STATUS "  BUILD_DAEMON: ${BUILD_DAEMON}")
message(STATUS "  BUILD_TESTS: ${BUILD_TESTS}")
message(STATUS "  BUILD_EXAMPLES: ${BUILD_EXAMPLES}")
message(STATUS "  ENABLE_CONTAINERS: ${ENABLE_CONTAINERS}")
message(STATUS "  ENABLE_MANAGEMENT_SERVER: ${ENABLE_MANAGEMENT_SERVER}")
message(STATUS "")
