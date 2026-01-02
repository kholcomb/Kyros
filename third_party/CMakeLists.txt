# Third-party dependencies

# Add nlohmann/json if not found system-wide
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann/json not found, using FetchContent to download")
    include(FetchContent)
    FetchContent_Declare(json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(json)
endif()

# CLI11 for command-line parsing (header-only)
message(STATUS "Using FetchContent to download CLI11")
include(FetchContent)
FetchContent_Declare(cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cli11)

# Google Test for unit testing
if(BUILD_TESTS)
    message(STATUS "Using FetchContent to download Google Test")
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
    )
    # Prevent Google Test from overriding our compiler/linker options
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    # Don't install Google Test
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # Enable testing
    include(GoogleTest)
endif()

# Note: In production, use proper package management:
# - vcpkg
# - Conan
# - System package manager
