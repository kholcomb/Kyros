#include <kyros/scan_types/config_scan.hpp>

namespace kyros {

ConfigScan::ConfigScan() {
    use_default_paths();
}

void ConfigScan::add_config_path(const std::string& path) {
    config_paths_.push_back(path);
}

void ConfigScan::add_config_paths(const std::vector<std::string>& paths) {
    config_paths_.insert(config_paths_.end(), paths.begin(), paths.end());
}

std::vector<std::string> ConfigScan::get_all_paths() const {
    return config_paths_;
}

void ConfigScan::use_default_paths() {
    config_paths_.clear();

    // Claude Desktop config (primary target)
    config_paths_.push_back("~/Library/Application Support/Claude/claude_desktop_config.json");  // macOS
    config_paths_.push_back("~/.config/Claude/claude_desktop_config.json");  // Linux

    // Common MCP server configuration locations
    config_paths_.push_back("~/.config/mcp/servers.json");
    config_paths_.push_back("~/.mcp/config.json");
    config_paths_.push_back("/etc/mcp/servers.json");
    config_paths_.push_back("./mcp.json");
    config_paths_.push_back("./servers.json");

    // VSCode MCP extension locations
    config_paths_.push_back("~/.vscode/mcp.json");
    config_paths_.push_back("~/.config/Code/User/mcp.json");

    // Project-specific locations
    config_paths_.push_back("./config/mcp.json");
    config_paths_.push_back("./config/servers.json");
}

} // namespace kyros
