#ifndef KYROS_CONFIG_SCAN_HPP
#define KYROS_CONFIG_SCAN_HPP

#include <kyros/scan_types/scan_type.hpp>

#include <string>
#include <vector>

namespace kyros {

/**
 * Configuration file scanning
 */
class ConfigScan : public ScanType {
public:
    ConfigScan();

    // Implements ScanType interface
    std::string name() const override { return "Configuration File Scan"; }
    bool is_available() const override { return true; }

    // ConfigScan-specific methods
    void add_config_path(const std::string& path);
    void add_config_paths(const std::vector<std::string>& paths);
    std::vector<std::string> get_all_paths() const;
    void use_default_paths();

private:
    std::vector<std::string> config_paths_;
};

} // namespace kyros

#endif // KYROS_CONFIG_SCAN_HPP
