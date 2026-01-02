#ifndef KYROS_CONFIG_DETECTION_ENGINE_HPP
#define KYROS_CONFIG_DETECTION_ENGINE_HPP

#include <kyros/detection/detection_engine.hpp>
#include <kyros/scan_types/config_scan.hpp>

#include <nlohmann/json.hpp>

namespace kyros {

// Server configuration from config file
struct ServerConfig {
    std::string name;
    std::string command;
    std::vector<std::string> args;
    std::map<std::string, std::string> env;
    std::string url;  // For HTTP transport
};

class ConfigDetectionEngine : public DetectionEngine {
public:
    explicit ConfigDetectionEngine(std::shared_ptr<ConfigScan> scan_type);

    std::string name() const override { return "ConfigDetectionEngine"; }
    std::vector<Candidate> detect() override;

    // Get statistics from last scan
    int get_last_scan_config_count() const { return last_scan_config_count_; }

private:
    std::shared_ptr<ConfigScan> scan_type_;

    // Helper methods
    std::vector<ServerConfig> parse_config_file(const std::string& path);
    Candidate create_candidate_from_config(const ServerConfig& config,
                                          const std::string& config_path);
    std::vector<Candidate> scan_claude_extensions();

    // Statistics from last scan
    int last_scan_config_count_ = 0;
};

} // namespace kyros

#endif
