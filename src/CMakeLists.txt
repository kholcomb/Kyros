# Main Kyros library

set(KYROS_SOURCES
    # Core
    scanner.cpp
    candidate.cpp
    evidence.cpp
    mcp_server.cpp
    rulepack.cpp

    # Detection Engines
    detection/detection_engine.cpp
    detection/config_detection_engine.cpp
    detection/process_detection_engine.cpp
    detection/network_detection_engine.cpp
    detection/container_detection_engine.cpp

    # Scan Types
    scan_types/scan_type.cpp
    scan_types/config_scan.cpp
    scan_types/process_scan.cpp
    scan_types/network_scan.cpp
    scan_types/container_scan.cpp

    # Testing Engines
    testing/testing_engine.cpp
    testing/stdio_testing_engine.cpp
    testing/http_testing_engine.cpp
    testing/protocol_detector.cpp
    testing/server_interrogator.cpp

    # Reporting
    reporting/reporter.cpp
    reporting/reporting_engine.cpp
    reporting/cli_reporter.cpp
    reporting/json_reporter.cpp
    reporting/html_reporter.cpp
    reporting/csv_reporter.cpp

    # Platform Abstraction
    platform/process.cpp

    # HTTP Client
    http/http_client.cpp
)

# Platform-specific sources
if(PLATFORM_LINUX)
    list(APPEND KYROS_SOURCES
        platform/linux/linux_platform_adapter.cpp
        platform/linux/linux_process.cpp
    )
elseif(PLATFORM_MACOS)
    list(APPEND KYROS_SOURCES
        platform/macos/macos_platform_adapter.cpp
        platform/macos/macos_process.cpp
    )
elseif(PLATFORM_WINDOWS)
    list(APPEND KYROS_SOURCES
        platform/windows/windows_platform_adapter.cpp
        platform/windows/windows_process.cpp
    )
endif()

# Daemon sources
if(BUILD_DAEMON)
    list(APPEND KYROS_SOURCES
        daemon/daemon.cpp
        daemon/scheduler.cpp
        daemon/state_store.cpp
        daemon/report_queue.cpp
    )

    if(PLATFORM_LINUX)
        list(APPEND KYROS_SOURCES
            daemon/platform/systemd_service.cpp
        )
    elseif(PLATFORM_WINDOWS)
        list(APPEND KYROS_SOURCES
            daemon/platform/windows_service.cpp
        )
    endif()
endif()

# Management server client
if(ENABLE_MANAGEMENT_SERVER)
    list(APPEND KYROS_SOURCES
        daemon/management_client.cpp
    )
endif()

# Create library
add_library(kyros-lib ${KYROS_SOURCES})

# Alias for consistency
add_library(kyros::lib ALIAS kyros-lib)

# Target properties
set_target_properties(kyros-lib PROPERTIES
    OUTPUT_NAME kyros
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Include directories
target_include_directories(kyros-lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(kyros-lib PUBLIC
    nlohmann_json::nlohmann_json
)

# Platform-specific libraries
if(PLATFORM_LINUX)
    target_link_libraries(kyros-lib PRIVATE
        pthread
        dl
    )
    if(BUILD_DAEMON)
        # systemd library for sd_notify
        find_library(SYSTEMD_LIB systemd)
        if(SYSTEMD_LIB)
            target_link_libraries(kyros-lib PRIVATE ${SYSTEMD_LIB})
            target_compile_definitions(kyros-lib PRIVATE HAVE_SYSTEMD)
        endif()
    endif()
elseif(PLATFORM_WINDOWS)
    target_link_libraries(kyros-lib PRIVATE
        ws2_32
        iphlpapi
        psapi
        advapi32
    )
endif()

# Daemon dependencies
if(BUILD_DAEMON)
    target_link_libraries(kyros-lib PRIVATE
        SQLite::SQLite3
    )
endif()

# Management server
if(ENABLE_MANAGEMENT_SERVER AND gRPC_FOUND)
    target_link_libraries(kyros-lib PRIVATE
        gRPC::grpc++
    )
    target_compile_definitions(kyros-lib PRIVATE ENABLE_MANAGEMENT_SERVER)
endif()

# Container support
if(ENABLE_CONTAINERS)
    target_compile_definitions(kyros-lib PRIVATE ENABLE_CONTAINERS)
endif()

# Install library
install(TARGETS kyros-lib
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Note: Export disabled for now due to FetchContent dependencies
# TODO: Re-enable when all dependencies are properly exportable
# install(EXPORT kyros-targets
#     FILE kyros-targets.cmake
#     NAMESPACE kyros::
#     DESTINATION lib/cmake/kyros
# )
