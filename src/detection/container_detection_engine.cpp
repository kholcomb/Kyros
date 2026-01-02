#include <kyros/detection/container_detection_engine.hpp>
#include <set>
#include <algorithm>

namespace kyros {

std::string ContainerDetectionEngine::name() const {
    return "ContainerDetectionEngine";
}

std::vector<Candidate> ContainerDetectionEngine::detect() {
    std::vector<Candidate> candidates;

    #ifdef ENABLE_CONTAINERS
    if (!platform_) {
        last_scan_container_count_ = 0;
        return candidates;
    }

    // Get MCP server list from docker mcp CLI
    auto mcp_server_ids = platform_->get_docker_mcp_servers();
    std::set<std::string> known_mcp_servers(mcp_server_ids.begin(),
                                             mcp_server_ids.end());

    // Get all containers
    auto containers = platform_->docker_list_containers();
    last_scan_container_count_ = static_cast<int>(containers.size());

    for (const auto& container : containers) {
        Candidate candidate;
        candidate.docker_container = container;
        candidate.process_name = container.image;
        candidate.command = container.command;

        // Add evidence based on what we find (engine is neutral)
        if (known_mcp_servers.count(container.id) > 0 ||
            known_mcp_servers.count(container.name) > 0) {
            // Note: Low confidence in engine, rulepack will boost to 1.0
            candidate.add_evidence(Evidence(
                "docker_mcp_server_list",
                "Container in docker mcp server list",
                0.5,  // Neutral - rulepack sets definitive value
                "docker-mcp-cli"
            ));
        }

        // Always check metadata (rulepacks determine importance)
        check_mcp_gateway(container, candidate);
        check_mcp_labels(container, candidate);
        check_mcp_entrypoint(container, candidate);
        check_mcp_environment(container, candidate);

        if (!candidate.evidence.empty()) {
            candidates.push_back(candidate);
        }
    }
    #endif

    return candidates;
}

void ContainerDetectionEngine::check_mcp_gateway(const DockerContainer& container, Candidate& candidate) {
    // Check for Docker MCP Gateway labels
    for (const auto& [key, value] : container.labels) {
        if (key.find("com.docker.mcp") == 0 || key.find("com.docker.mcp-gateway") == 0) {
            candidate.add_evidence(Evidence(
                "container_mcp_gateway",
                "Docker MCP Gateway label: " + key + "=" + value,
                0.5,  // Neutral - rulepack sets to 0.95
                "container:" + container.id
            ));
            break;  // One Gateway label is enough
        }
    }
}

void ContainerDetectionEngine::check_mcp_labels(const DockerContainer& container, Candidate& candidate) {
    for (const auto& [key, value] : container.labels) {
        std::string key_lower = key;
        std::string value_lower = value;
        std::transform(key_lower.begin(), key_lower.end(), key_lower.begin(), ::tolower);
        std::transform(value_lower.begin(), value_lower.end(), value_lower.begin(), ::tolower);

        // Boolean MCP labels
        if ((key_lower == "mcp" || key_lower == "mcp-server" || key_lower == "mcp.enabled") &&
            (value_lower == "true" || value_lower == "1" || value_lower == "yes")) {
            candidate.add_evidence(Evidence(
                "container_label_mcp_bool",
                "Explicit MCP label: " + key + "=" + value,
                0.5,  // Neutral - rulepack sets to 0.90
                "container:" + container.id
            ));
        }

        // Type labels
        if (key_lower == "mcp.type" || key_lower == "mcp.role") {
            if (value_lower == "server") {
                candidate.add_evidence(Evidence(
                    "container_label_mcp_type",
                    "MCP type label: " + key + "=" + value,
                    0.5,  // Neutral - rulepack sets to 0.85
                    "container:" + container.id
                ));
            }
        }

        // Transport labels
        if (key_lower == "mcp.transport") {
            if (value_lower == "http" || value_lower == "stdio" || value_lower == "sse") {
                candidate.add_evidence(Evidence(
                    "container_label_mcp_transport",
                    "MCP transport label: " + key + "=" + value,
                    0.5,  // Neutral - rulepack sets to 0.75
                    "container:" + container.id
                ));

                // Set transport hint
                if (value_lower == "http") {
                    candidate.transport_hint = TransportType::Http;
                } else if (value_lower == "stdio") {
                    candidate.transport_hint = TransportType::Stdio;
                } else if (value_lower == "sse") {
                    candidate.transport_hint = TransportType::Sse;
                }
            }
        }
    }
}

void ContainerDetectionEngine::check_mcp_entrypoint(const DockerContainer& container, Candidate& candidate) {
    // Check if entrypoint contains known MCP server patterns
    std::string entrypoint_lower = container.entrypoint_path;
    std::transform(entrypoint_lower.begin(), entrypoint_lower.end(), entrypoint_lower.begin(), ::tolower);

    // Known MCP server executable patterns
    const std::vector<std::string> mcp_patterns = {
        "@modelcontextprotocol/",  // Node.js MCP packages
        "mcp-server-",              // Generic MCP server binaries
        "/app/mcp",                 // Common MCP app path
        "mcp_server",               // Python-style naming
        "/mcp/",                    // MCP directory in path
    };

    for (const auto& pattern : mcp_patterns) {
        if (entrypoint_lower.find(pattern) != std::string::npos) {
            candidate.add_evidence(Evidence(
                "container_entrypoint_mcp",
                "Known MCP server in entrypoint: " + container.entrypoint_path,
                0.5,  // Neutral - rulepack sets to 0.85
                "container:" + container.id
            ));
            break;  // One match is enough
        }
    }

    // Also check in args
    for (const auto& arg : container.entrypoint_args) {
        std::string arg_lower = arg;
        std::transform(arg_lower.begin(), arg_lower.end(), arg_lower.begin(), ::tolower);

        for (const auto& pattern : mcp_patterns) {
            if (arg_lower.find(pattern) != std::string::npos) {
                candidate.add_evidence(Evidence(
                    "container_entrypoint_mcp",
                    "Known MCP server in arguments: " + arg,
                    0.5,  // Neutral - rulepack sets to 0.85
                    "container:" + container.id
                ));
                return;  // One match is enough
            }
        }
    }
}

void ContainerDetectionEngine::check_mcp_environment(const DockerContainer& container, Candidate& candidate) {
    for (const auto& [key, value] : container.env) {
        std::string value_lower = value;
        std::transform(value_lower.begin(), value_lower.end(), value_lower.begin(), ::tolower);

        // Boolean environment variables
        if ((key == "MCP_ENABLED" || key == "MCP_SERVER") &&
            (value_lower == "true" || value_lower == "1" || value_lower == "yes")) {
            candidate.add_evidence(Evidence(
                "container_env_mcp_bool",
                "Explicit MCP environment: " + key + "=" + value,
                0.5,  // Neutral - rulepack sets to 0.70
                "container:" + container.id
            ));
        }

        // Config environment variables
        if (key == "MCP_TRANSPORT" || key == "MCP_PORT" || key == "MCP_SERVER_NAME") {
            candidate.add_evidence(Evidence(
                "container_env_mcp_config",
                "MCP config environment: " + key + "=" + value,
                0.5,  // Neutral - rulepack sets to 0.65
                "container:" + container.id
            ));

            // Set transport hint from MCP_TRANSPORT
            if (key == "MCP_TRANSPORT") {
                if (value_lower == "http") {
                    candidate.transport_hint = TransportType::Http;
                } else if (value_lower == "stdio") {
                    candidate.transport_hint = TransportType::Stdio;
                } else if (value_lower == "sse") {
                    candidate.transport_hint = TransportType::Sse;
                }
            }
        }
    }
}

} // namespace kyros
