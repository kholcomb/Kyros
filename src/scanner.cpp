#include <kyros/scanner.hpp>
#include <kyros/rulepack.hpp>
#include <kyros/detection/detection_engine.hpp>
#include <kyros/detection/config_detection_engine.hpp>
#include <kyros/detection/process_detection_engine.hpp>
#include <kyros/detection/network_detection_engine.hpp>
#include <kyros/detection/container_detection_engine.hpp>
#include <kyros/testing/stdio_testing_engine.hpp>
#include <kyros/testing/http_testing_engine.hpp>
#include <kyros/testing/server_interrogator.hpp>
#include <kyros/reporting/reporting_engine.hpp>
#include <kyros/reporting/cli_reporter.hpp>
#include <kyros/reporting/json_reporter.hpp>
#include <kyros/reporting/html_reporter.hpp>
#include <kyros/reporting/csv_reporter.hpp>
#include <kyros/platform/platform_adapter.hpp>
#include <kyros/scan_types/config_scan.hpp>
#include <kyros/http/http_client.hpp>

#include <chrono>
#include <algorithm>
#include <iostream>
#include <filesystem>

namespace kyros {

// Helper function to deduplicate candidates
namespace {
    void deduplicate_candidates(std::vector<Candidate>& candidates) {
        // Remove duplicates based on unique identifiers
        auto it = candidates.begin();
        while (it != candidates.end()) {
            auto duplicate = std::find_if(it + 1, candidates.end(),
                [&it](const Candidate& other) {
                    // Same config file + key
                    if (!it->config_file.empty() && !other.config_file.empty()) {
                        if (it->config_file == other.config_file &&
                            it->config_key == other.config_key) {
                            return true;
                        }
                    }
                    // Same PID
                    if (it->pid > 0 && other.pid > 0) {
                        if (it->pid == other.pid) {
                            return true;
                        }
                    }
                    // Same URL
                    if (!it->url.empty() && !other.url.empty()) {
                        if (it->url == other.url) {
                            return true;
                        }
                    }
                    // Same command (exact match)
                    if (!it->command.empty() && !other.command.empty()) {
                        if (it->command == other.command) {
                            return true;
                        }
                    }
                    return false;
                });

            if (duplicate != candidates.end()) {
                // Merge evidence from duplicate into the first one
                for (auto& evidence : duplicate->evidence) {
                    it->add_evidence(evidence);
                }
                // Remove the duplicate
                candidates.erase(duplicate);
            } else {
                ++it;
            }
        }
    }
}

// Scanner::Impl - Private implementation
class Scanner::Impl {
public:
    std::unique_ptr<ReportingEngine> reporting_engine;
    std::shared_ptr<PlatformAdapter> platform_adapter;
    PassiveScanner passive_scanner;
    ActiveScanner active_scanner;

    Impl() {
        reporting_engine = std::make_unique<ReportingEngine>();
        platform_adapter = create_platform_adapter();

        // Register all reporters
        reporting_engine->register_reporter(std::make_shared<cliReporter>());
        reporting_engine->register_reporter(std::make_shared<jsonReporter>());
        reporting_engine->register_reporter(std::make_shared<htmlReporter>());
        reporting_engine->register_reporter(std::make_shared<csvReporter>());

        // Set platform adapters on scanners
        passive_scanner.set_platform_adapter(platform_adapter);
        active_scanner.set_platform_adapter(platform_adapter);
    }
};

Scanner::Scanner() : impl_(std::make_unique<Impl>()) {}
Scanner::~Scanner() = default;

ScanResults Scanner::scan(const ScanConfig& config) {
    ScanResults results;
    results.verbose = config.verbose;

    try {
        // Phase 1: Passive Scan (unless ActiveOnly mode)
        if (config.mode != ScanMode::ActiveOnly) {
            results.passive_results = impl_->passive_scanner.scan(config.passive_config);

            // Merge passive scan errors
            for (const auto& error : results.passive_results.errors) {
                results.errors.push_back("Passive scan: " + error);
            }
        }

        // Phase 2: Active Scan (if active mode enabled)
        if (config.mode == ScanMode::PassiveThenActive ||
            config.mode == ScanMode::ActiveOnly) {

            // Get candidates to test
            std::vector<Candidate> candidates_to_test;

            if (config.mode == ScanMode::PassiveThenActive) {
                // Use candidates from passive scan
                candidates_to_test = results.passive_results.candidates;
            } else {
                // ActiveOnly mode - no candidates from passive scan
                // This mode is for testing pre-provided candidates
                // For now, just use empty vector
                // TODO: Add way to provide candidates externally
            }

            // Run active scan if we have candidates
            if (!candidates_to_test.empty() || config.mode == ScanMode::ActiveOnly) {
                auto active_results = impl_->active_scanner.scan(
                    candidates_to_test,
                    config.active_config
                );

                // Merge active scan errors
                for (const auto& error : active_results.errors) {
                    results.errors.push_back("Active scan: " + error);
                }

                results.active_results = std::move(active_results);
            }
        }

    } catch (const std::exception& e) {
        // Collect errors
        results.errors.push_back(std::string("Scan error: ") + e.what());
    }

    return results;
}

ReportingEngine& Scanner::reporting_engine() {
    return *impl_->reporting_engine;
}

const ReportingEngine& Scanner::reporting_engine() const {
    return *impl_->reporting_engine;
}

void Scanner::set_platform_adapter(std::shared_ptr<PlatformAdapter> adapter) {
    impl_->platform_adapter = adapter;
}

void Scanner::load_rulepack(const std::string& path) {
    impl_->passive_scanner.load_rulepack(path);
}

void Scanner::load_default_rulepacks() {
    impl_->passive_scanner.load_default_rulepacks();
}

// PassiveScanner
PassiveScanner::PassiveScanner() : rule_engine_(std::make_unique<RuleEngine>()) {
    // Load default rulepacks on initialization
    load_default_rulepacks();
}
PassiveScanner::~PassiveScanner() = default;

PassiveScanResults PassiveScanner::scan(const PassiveScanConfig& config) {
    auto start_time = std::chrono::system_clock::now();
    PassiveScanResults results;
    results.scan_timestamp = start_time;

    // Initialize engines if not done yet
    if (engines_.empty()) {
        initialize_engines();
    }

    // Run all detection engines
    for (auto& engine : engines_) {
        try {
            auto candidates = engine->detect();

            // Apply rulepacks before filtering
            for (auto& candidate : candidates) {
                rule_engine_->apply(candidate);
            }

            // Filter by confidence threshold
            for (auto& candidate : candidates) {
                if (candidate.confidence_score >= config.min_confidence) {
                    results.candidates.push_back(std::move(candidate));
                }
            }

            // Update statistics
            if (engine->name() == "ConfigDetectionEngine") {
                auto* config_engine = dynamic_cast<ConfigDetectionEngine*>(engine.get());
                if (config_engine) {
                    results.config_files_checked += config_engine->get_last_scan_config_count();
                }
            } else if (engine->name() == "ProcessDetectionEngine") {
                auto* process_engine = dynamic_cast<ProcessDetectionEngine*>(engine.get());
                if (process_engine) {
                    results.processes_scanned += process_engine->get_last_scan_process_count();
                }
            } else if (engine->name() == "NetworkDetectionEngine") {
                auto* network_engine = dynamic_cast<NetworkDetectionEngine*>(engine.get());
                if (network_engine) {
                    results.network_sockets_checked += network_engine->get_last_scan_socket_count();
                }
            }
        } catch (const std::exception& e) {
            // Continue on error - don't fail entire scan
            std::string error_msg = std::string("Error in ") + engine->name() + ": " + e.what();
            results.errors.push_back(error_msg);
        }
    }

    // Deduplicate candidates
    deduplicate_candidates(results.candidates);

    // Enforce max_candidates limit
    if (results.candidates.size() > static_cast<size_t>(config.max_candidates)) {
        // Keep highest confidence candidates
        std::partial_sort(results.candidates.begin(),
                         results.candidates.begin() + config.max_candidates,
                         results.candidates.end(),
                         [](const Candidate& a, const Candidate& b) {
                             return a.confidence_score > b.confidence_score;
                         });
        results.candidates.resize(config.max_candidates);
    }

    // Calculate scan duration
    auto end_time = std::chrono::system_clock::now();
    Duration duration = end_time - start_time;
    results.scan_duration_seconds = duration.count();

    return results;
}

void PassiveScanner::set_platform_adapter(std::shared_ptr<PlatformAdapter> adapter) {
    platform_ = adapter;
}

void PassiveScanner::initialize_engines() {
    // Create ConfigScan with default paths
    auto config_scan = std::make_shared<ConfigScan>();
    config_scan->use_default_paths();

    // Create ConfigDetectionEngine
    auto config_engine = std::make_unique<ConfigDetectionEngine>(config_scan);
    config_engine->set_platform_adapter(platform_);
    engines_.push_back(std::move(config_engine));

    // Phase 2: Add ProcessDetectionEngine
    auto process_engine = std::make_unique<ProcessDetectionEngine>();
    process_engine->set_platform_adapter(platform_);
    engines_.push_back(std::move(process_engine));

    // Phase 2: Add NetworkDetectionEngine
    auto network_engine = std::make_unique<NetworkDetectionEngine>();
    network_engine->set_platform_adapter(platform_);
    engines_.push_back(std::move(network_engine));

    // Phase 2: Add ContainerDetectionEngine
    auto container_engine = std::make_unique<ContainerDetectionEngine>();
    container_engine->set_platform_adapter(platform_);
    engines_.push_back(std::move(container_engine));
}

void PassiveScanner::load_rulepack(const std::string& path) {
    try {
        rule_engine_->load_rulepack(path);
    } catch (const std::exception& e) {
        // Log error but don't fail - rulepacks are optional
        std::cerr << "Warning: Failed to load rulepack " << path << ": " << e.what() << "\n";
    }
}

void PassiveScanner::load_default_rulepacks() {
    // Try to load default detection rulepack from common locations
    std::vector<std::string> default_rulepack_paths = {
        "config/rulepacks/default.json",
        "./config/rulepacks/default.json",
        "../config/rulepacks/default.json",
        "/usr/local/share/kyros/rulepacks/default.json",
        "/usr/share/kyros/rulepacks/default.json"
    };

    for (const auto& path : default_rulepack_paths) {
        if (std::filesystem::exists(path)) {
            load_rulepack(path);
            break;  // Load only the first found default rulepack
        }
    }

    // Try to load exclusion rulepack from common locations
    std::vector<std::string> exclusion_rulepack_paths = {
        "config/rulepacks/exclusions.json",
        "./config/rulepacks/exclusions.json",
        "../config/rulepacks/exclusions.json",
        "/usr/local/share/kyros/rulepacks/exclusions.json",
        "/usr/share/kyros/rulepacks/exclusions.json"
    };

    for (const auto& path : exclusion_rulepack_paths) {
        if (std::filesystem::exists(path)) {
            load_rulepack(path);
            break;  // Load only the first found exclusion rulepack
        }
    }
}

// ActiveScanner
ActiveScanner::ActiveScanner() {}
ActiveScanner::~ActiveScanner() = default;

ActiveScanResults ActiveScanner::scan(
    const std::vector<Candidate>& candidates,
    const ActiveScanConfig& config) {
    auto start_time = std::chrono::system_clock::now();
    ActiveScanResults results;
    results.scan_timestamp = start_time;
    results.candidates_tested = candidates;

    // Initialize engines if not done yet
    if (testing_engines_.empty()) {
        initialize_engines();
    }

    // Set timeout for all testing engines
    auto timeout = std::chrono::milliseconds(config.probe_timeout_ms);
    for (auto& engine : testing_engines_) {
        engine->set_timeout(timeout);
    }

    // Test each candidate
    for (const auto& candidate : candidates) {
        // Skip if in skip lists
        if (candidate.pid > 0 &&
            std::find(config.skip_pids.begin(), config.skip_pids.end(),
                     candidate.pid) != config.skip_pids.end()) {
            continue;
        }
        if (!candidate.url.empty() &&
            std::find(config.skip_urls.begin(), config.skip_urls.end(),
                     candidate.url) != config.skip_urls.end()) {
            continue;
        }

        bool confirmed = false;
        results.candidates_tested_count++;
        std::vector<std::string> engine_errors;

        // Try each testing engine
        for (auto& engine : testing_engines_) {
            try {
                auto server_opt = engine->test(candidate);
                if (server_opt.has_value()) {
                    // Test succeeded!
                    auto server = server_opt.value();

                    // Store the candidate information in the server
                    server.candidate = candidate;

                    // Interrogate the server if enabled
                    if (config.interrogate && config.interrogation_config.interrogate_enabled) {
                        // Create interrogator if not already created
                        if (!interrogator_) {
                            interrogator_ = std::make_unique<ServerInterrogator>(
                                config.interrogation_config,
                                platform_,
                                http_client_
                            );
                        }
                        // Perform interrogation
                        interrogator_->interrogate(server);
                    }

                    results.confirmed_servers.push_back(std::move(server));
                    results.servers_confirmed_count++;
                    confirmed = true;
                    break;  // Don't try other engines for this candidate
                }
            } catch (const std::exception& e) {
                // Collect error for this engine
                engine_errors.push_back(std::string(engine->name()) + ": " + e.what());
            }
        }

        // If no engine succeeded, add to failed tests and log errors
        if (!confirmed) {
            results.failed_tests.push_back(candidate);
            if (!engine_errors.empty()) {
                std::string error_msg = "Failed to test candidate";
                if (!candidate.command.empty()) {
                    error_msg += " (command: " + candidate.command + ")";
                } else if (!candidate.url.empty()) {
                    error_msg += " (url: " + candidate.url + ")";
                }
                error_msg += " - Errors: ";
                for (size_t i = 0; i < engine_errors.size(); i++) {
                    if (i > 0) error_msg += "; ";
                    error_msg += engine_errors[i];
                }
                results.errors.push_back(error_msg);
            }
            results.tests_failed_count++;
        }
    }

    // Calculate scan duration
    auto end_time = std::chrono::system_clock::now();
    Duration duration = end_time - start_time;
    results.scan_duration_seconds = duration.count();

    return results;
}

void ActiveScanner::set_platform_adapter(std::shared_ptr<PlatformAdapter> adapter) {
    platform_ = adapter;
}

void ActiveScanner::initialize_engines() {
    // Create shared HttpClient (used by both HttpTestingEngine and ServerInterrogator)
    http_client_ = std::make_shared<HttpClient>();

    // Create StdioTestingEngine
    auto stdio_engine = std::make_unique<StdioTestingEngine>(platform_);
    testing_engines_.push_back(std::move(stdio_engine));

    // Create HttpTestingEngine
    auto http_engine = std::make_unique<HttpTestingEngine>(http_client_);
    testing_engines_.push_back(std::move(http_engine));
}

} // namespace kyros
