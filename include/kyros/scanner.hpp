#ifndef KYROS_SCANNER_HPP
#define KYROS_SCANNER_HPP

#include <kyros/config.hpp>
#include <kyros/types.hpp>

#include <memory>
#include <vector>

namespace kyros {

// Forward declarations
class DetectionEngine;
class TestingEngine;
class ReportingEngine;
class PlatformAdapter;
class ServerInterrogator;
class HttpClient;

/**
 * Main Kyros scanner
 *
 * Orchestrates passive detection, active probing, and reporting
 */
class Scanner {
public:
    Scanner();
    ~Scanner();

    // Main scan entry point
    ScanResults scan(const ScanConfig& config);

    // Component access (for advanced usage)
    ReportingEngine& reporting_engine();
    const ReportingEngine& reporting_engine() const;

    // Configuration
    void set_platform_adapter(std::shared_ptr<PlatformAdapter> adapter);

    // Rulepack management
    void load_rulepack(const std::string& path);
    void load_default_rulepacks();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Forward declaration
class RuleEngine;

/**
 * Passive scanner (for internal use)
 */
class PassiveScanner {
public:
    PassiveScanner();
    ~PassiveScanner();

    PassiveScanResults scan(const PassiveScanConfig& config);

    void set_platform_adapter(std::shared_ptr<PlatformAdapter> adapter);

    // Load rulepacks
    void load_rulepack(const std::string& path);
    void load_default_rulepacks();

private:
    std::shared_ptr<PlatformAdapter> platform_;
    std::vector<std::unique_ptr<DetectionEngine>> engines_;
    std::unique_ptr<RuleEngine> rule_engine_;

    void initialize_engines();
};

/**
 * Active scanner (for internal use)
 */
class ActiveScanner {
public:
    ActiveScanner();
    ~ActiveScanner();

    ActiveScanResults scan(
        const std::vector<Candidate>& candidates,
        const ActiveScanConfig& config);

    void set_platform_adapter(std::shared_ptr<PlatformAdapter> adapter);

private:
    std::shared_ptr<PlatformAdapter> platform_;
    std::shared_ptr<HttpClient> http_client_;
    std::vector<std::unique_ptr<TestingEngine>> testing_engines_;
    std::unique_ptr<ServerInterrogator> interrogator_;

    void initialize_engines();
};

} // namespace kyros

#endif // KYROS_SCANNER_HPP
