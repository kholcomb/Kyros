#include <kyros/detection/config_detection_engine.hpp>
#include <kyros/scan_types/config_scan.hpp>

#include <iostream>
#include <sstream>

namespace kyros {

ConfigDetectionEngine::ConfigDetectionEngine(std::shared_ptr<ConfigScan> scan_type)
    : scan_type_(scan_type) {}

std::vector<Candidate> ConfigDetectionEngine::detect() {
    std::vector<Candidate> candidates;
    last_scan_config_count_ = 0;

    if (!scan_type_ || !scan_type_->is_enabled()) {
        return candidates;
    }

    if (!platform_) {
        return candidates;
    }

    // Get all config paths to scan
    auto config_paths = scan_type_->get_all_paths();

    for (const auto& path : config_paths) {
        try {
            // Expand path (handle ~ and environment variables)
            std::string expanded_path = platform_->expand_path(path);

            // Check if file exists
            if (!platform_->file_exists(expanded_path)) {
                continue;
            }

            // Count this config file as checked
            last_scan_config_count_++;

            // Parse config file
            auto server_configs = parse_config_file(expanded_path);

            // Create candidate for each server found
            for (const auto& config : server_configs) {
                auto candidate = create_candidate_from_config(config, expanded_path);
                candidates.push_back(candidate);
            }

        } catch (const std::exception& e) {
            // Log error but continue processing other files
            std::cerr << "Error processing config file " << path << ": "
                     << e.what() << std::endl;
        }
    }

    // Scan Claude Extensions directory
    auto extension_candidates = scan_claude_extensions();
    candidates.insert(candidates.end(), extension_candidates.begin(), extension_candidates.end());

    return candidates;
}

std::vector<ServerConfig> ConfigDetectionEngine::parse_config_file(const std::string& path) {
    std::vector<ServerConfig> configs;

    try {
        // Read and parse JSON file
        auto json = platform_->read_json_file(path);

        // Look for "mcpServers" section (Claude Desktop format)
        if (json.contains("mcpServers") && json["mcpServers"].is_object()) {
            for (auto& [server_name, server_obj] : json["mcpServers"].items()) {
                ServerConfig config;
                config.name = server_name;

                // Extract command (required)
                if (server_obj.contains("command") && server_obj["command"].is_string()) {
                    config.command = server_obj["command"].get<std::string>();
                } else {
                    continue;  // Skip servers without command
                }

                // Extract args (optional)
                if (server_obj.contains("args") && server_obj["args"].is_array()) {
                    for (const auto& arg : server_obj["args"]) {
                        if (arg.is_string()) {
                            config.args.push_back(arg.get<std::string>());
                        }
                    }
                }

                // Extract env (optional)
                if (server_obj.contains("env") && server_obj["env"].is_object()) {
                    for (auto& [key, value] : server_obj["env"].items()) {
                        if (value.is_string()) {
                            config.env[key] = value.get<std::string>();
                        }
                    }
                }

                // Extract URL (optional, for HTTP transport)
                if (server_obj.contains("url") && server_obj["url"].is_string()) {
                    config.url = server_obj["url"].get<std::string>();
                }

                configs.push_back(config);
            }
        }

        // Also support alternative format with "servers" array
        if (json.contains("servers") && json["servers"].is_array()) {
            for (const auto& server_obj : json["servers"]) {
                ServerConfig config;

                // Extract name
                if (server_obj.contains("name") && server_obj["name"].is_string()) {
                    config.name = server_obj["name"].get<std::string>();
                }

                // Extract command (required)
                if (server_obj.contains("command") && server_obj["command"].is_string()) {
                    config.command = server_obj["command"].get<std::string>();
                } else {
                    continue;
                }

                // Extract args, env, url (same as above)
                if (server_obj.contains("args") && server_obj["args"].is_array()) {
                    for (const auto& arg : server_obj["args"]) {
                        if (arg.is_string()) {
                            config.args.push_back(arg.get<std::string>());
                        }
                    }
                }

                if (server_obj.contains("env") && server_obj["env"].is_object()) {
                    for (auto& [key, value] : server_obj["env"].items()) {
                        if (value.is_string()) {
                            config.env[key] = value.get<std::string>();
                        }
                    }
                }

                if (server_obj.contains("url") && server_obj["url"].is_string()) {
                    config.url = server_obj["url"].get<std::string>();
                }

                configs.push_back(config);
            }
        }

    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse config file: " + std::string(e.what()));
    }

    return configs;
}

Candidate ConfigDetectionEngine::create_candidate_from_config(
    const ServerConfig& config,
    const std::string& config_path) {

    Candidate candidate;

    // Set config metadata
    candidate.config_file = config_path;
    candidate.config_key = config.name;

    // Build full command from command + args
    if (!config.command.empty()) {
        std::ostringstream cmd_stream;
        cmd_stream << config.command;
        for (const auto& arg : config.args) {
            cmd_stream << " " << arg;
        }
        candidate.command = cmd_stream.str();
    }

    // Set environment variables
    candidate.environment = config.env;

    // Set URL if HTTP transport
    if (!config.url.empty()) {
        candidate.url = config.url;
        candidate.transport_hint = TransportType::Http;
    } else {
        candidate.transport_hint = TransportType::Stdio;
    }

    // Add evidence
    Evidence config_evidence(
        "config_declared",
        "Declared in config file: " + config_path,
        0.9,  // High confidence - explicitly configured
        config_path
    );
    candidate.add_evidence(config_evidence);

    return candidate;
}

std::vector<Candidate> ConfigDetectionEngine::scan_claude_extensions() {
    std::vector<Candidate> candidates;

    // Claude Extensions directory paths (platform-specific)
    std::vector<std::string> extension_base_paths = {
        "~/Library/Application Support/Claude/Claude Extensions",  // macOS
        "~/.config/Claude/Claude Extensions",  // Linux
    };

    for (const auto& base_path : extension_base_paths) {
        try {
            std::string expanded_base = platform_->expand_path(base_path);

            // Check if directory exists
            if (!platform_->file_exists(expanded_base)) {
                continue;
            }

            // List all subdirectories (each is an installed extension)
            auto extension_dirs = platform_->list_directory(expanded_base);

            for (const auto& extension_name : extension_dirs) {
                try {
                    std::string extension_path = expanded_base + "/" + extension_name;

                    // Check if it's a directory (skip files)
                    if (!platform_->file_exists(extension_path)) {
                        continue;
                    }

                    // Look for entry point: dist/index.js (most common)
                    std::string entry_point = extension_path + "/dist/index.js";

                    // Also check for alternative entry points
                    if (!platform_->file_exists(entry_point)) {
                        entry_point = extension_path + "/index.js";
                    }
                    if (!platform_->file_exists(entry_point)) {
                        entry_point = extension_path + "/build/index.js";
                    }

                    // If no entry point found, skip
                    if (!platform_->file_exists(entry_point)) {
                        std::cerr << "Warning: Claude Extension " << extension_name
                                  << " found but no entry point detected" << std::endl;
                        continue;
                    }

                    // Create candidate
                    Candidate candidate;
                    candidate.config_file = extension_path;
                    candidate.config_key = extension_name;
                    candidate.command = "node " + entry_point;
                    candidate.transport_hint = TransportType::Stdio;

                    // Add evidence
                    Evidence extension_evidence(
                        "claude_extension_installed",
                        "Installed as Claude Desktop Extension: " + extension_path,
                        0.95,  // Very high confidence - explicitly installed by Claude Desktop
                        extension_path
                    );
                    candidate.add_evidence(extension_evidence);

                    candidates.push_back(candidate);
                    last_scan_config_count_++;  // Count as a config source

                } catch (const std::exception& e) {
                    std::cerr << "Error processing extension " << extension_name << ": "
                             << e.what() << std::endl;
                }
            }

        } catch (const std::exception& e) {
            // Directory doesn't exist or can't be read - skip silently
            continue;
        }
    }

    return candidates;
}

} // namespace kyros
