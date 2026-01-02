#ifndef KYROS_HPP
#define KYROS_HPP

/**
 * Kyros: Model Context Protocol Server Detection Engine
 *
 * Main public API header
 */

// Version information
#include <kyros/version.hpp>

// Core types
#include <kyros/types.hpp>
#include <kyros/evidence.hpp>
#include <kyros/candidate.hpp>
#include <kyros/mcp_server.hpp>

// Scanner
#include <kyros/scanner.hpp>

// Configuration
#include <kyros/config.hpp>

// Scan types
#include <kyros/scan_types/scan_type.hpp>
#include <kyros/scan_types/config_scan.hpp>
#include <kyros/scan_types/process_scan.hpp>
#include <kyros/scan_types/network_scan.hpp>
#include <kyros/scan_types/container_scan.hpp>

// Detection engines
#include <kyros/detection/detection_engine.hpp>
#include <kyros/detection/config_detection_engine.hpp>
#include <kyros/detection/process_detection_engine.hpp>
#include <kyros/detection/network_detection_engine.hpp>
#include <kyros/detection/container_detection_engine.hpp>

// Testing engines
#include <kyros/testing/testing_engine.hpp>
#include <kyros/testing/stdio_testing_engine.hpp>
#include <kyros/testing/http_testing_engine.hpp>
#include <kyros/testing/server_interrogator.hpp>

// Reporting
#include <kyros/reporting/reporter.hpp>
#include <kyros/reporting/reporting_engine.hpp>
#include <kyros/reporting/cli_reporter.hpp>
#include <kyros/reporting/json_reporter.hpp>
#include <kyros/reporting/html_reporter.hpp>
#include <kyros/reporting/csv_reporter.hpp>

// Platform abstraction
#include <kyros/platform/platform_adapter.hpp>
#include <kyros/platform/process.hpp>

#ifdef ENABLE_DAEMON
#include <kyros/daemon/daemon.hpp>
#include <kyros/daemon/state_store.hpp>
#endif

#endif // KYROS_HPP
