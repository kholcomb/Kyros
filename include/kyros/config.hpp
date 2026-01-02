#ifndef KYROS_CONFIG_HPP
#define KYROS_CONFIG_HPP

#include <kyros/types.hpp>
#include <kyros/candidate.hpp>
#include <kyros/mcp_server.hpp>

#include <chrono>
#include <string>
#include <vector>

namespace kyros {

/**
 * Passive scan configuration
 */
struct PassiveScanConfig {
    // What to scan
    bool scan_configs = true;
    bool scan_processes = true;
    bool scan_network = true;
    bool scan_containers = false;  // May require elevated privileges

    // Evidence thresholds
    double min_confidence = 0.0;

    // Performance
    int max_candidates = 1000;

    // Config file paths (empty = use defaults)
    std::vector<std::string> additional_config_paths;
};

/**
 * Interrogation configuration
 */
struct InterrogationConfig {
    bool interrogate_enabled = false;

    // Specific aspects to interrogate
    bool get_tools = true;
    bool get_resources = true;
    bool get_resource_templates = true;
    bool get_prompts = true;

    // Limits (to prevent hanging on huge responses)
    int max_tools = 100;
    int max_resources = 100;
    int max_prompts = 50;

    // Timeout for each interrogation request
    std::chrono::milliseconds timeout{5000};
};

/**
 * Active scan configuration
 */
struct ActiveScanConfig {
    // Testing options
    int probe_timeout_ms = 5000;
    int max_parallel_probes = 10;
    bool test_all_candidates = true;

    // Interrogation options
    bool interrogate = false;
    InterrogationConfig interrogation_config;

    // Safety
    bool require_confirmation = false;
    std::vector<int> skip_pids;
    std::vector<std::string> skip_urls;
};

/**
 * Scan results (passive mode)
 */
struct PassiveScanResults {
    std::vector<Candidate> candidates;

    // Statistics
    int config_files_checked = 0;
    int processes_scanned = 0;
    int network_sockets_checked = 0;
    int containers_scanned = 0;

    double scan_duration_seconds = 0.0;
    Timestamp scan_timestamp;

    // Errors encountered during passive scan
    std::vector<std::string> errors;
};

/**
 * Scan results (active mode)
 */
struct ActiveScanResults {
    // Input
    std::vector<Candidate> candidates_tested;

    // Output
    std::vector<MCPServer> confirmed_servers;
    std::vector<Candidate> failed_tests;

    // Statistics
    int candidates_tested_count = 0;
    int servers_confirmed_count = 0;
    int tests_failed_count = 0;
    double scan_duration_seconds = 0.0;
    Timestamp scan_timestamp;

    // Errors encountered during active scan
    std::vector<std::string> errors;
};

/**
 * Combined scan results
 */
struct ScanResults {
    // Passive results (always present unless ActiveOnly mode)
    PassiveScanResults passive_results;

    // Active results (only if active mode enabled)
    std::optional<ActiveScanResults> active_results;

    // Errors encountered during scan
    std::vector<std::string> errors;

    // Configuration options
    bool verbose = false;

    // Convenience accessors
    const std::vector<Candidate>& candidates() const {
        return passive_results.candidates;
    }

    const std::vector<MCPServer>& confirmed_servers() const {
        static const std::vector<MCPServer> empty;
        return active_results ? active_results->confirmed_servers : empty;
    }

    bool has_active_results() const {
        return active_results.has_value();
    }
};

/**
 * Overall scan configuration
 */
struct ScanConfig {
    // Mode selection
    ScanMode mode = ScanMode::PassiveOnly;

    // Passive configuration
    PassiveScanConfig passive_config;

    // Active configuration
    ActiveScanConfig active_config;

    // Output options
    bool verbose = false;
    std::string output_format = "cli";
    std::string output_file;
};

} // namespace kyros

#endif // KYROS_CONFIG_HPP
