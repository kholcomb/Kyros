#ifndef KYROS_CANDIDATE_HPP
#define KYROS_CANDIDATE_HPP

#include <kyros/evidence.hpp>
#include <kyros/types.hpp>

#include <string>
#include <vector>

namespace kyros {

/**
 * A candidate MCP server (not yet confirmed)
 */
struct Candidate {
    // Process information (for stdio transport)
    int pid = 0;
    std::string command;
    std::string process_name;
    int parent_pid = 0;
    std::map<std::string, std::string> environment;

    // Configuration file (if found)
    std::string config_file;
    std::string config_key;  // Key within config file

    // Network information (for HTTP/SSE transport)
    std::string url;
    std::string address;
    int port = 0;

    // Container information (if applicable)
    std::optional<DockerContainer> docker_container;
    std::optional<KubernetesPod> k8s_pod;

    // Detection metadata
    std::vector<Evidence> evidence;
    double confidence_score = 0.0;
    TransportType transport_hint = TransportType::Unknown;

    // Helpers
    bool is_config_candidate() const { return !config_file.empty(); }
    bool is_process_candidate() const { return pid > 0; }
    bool is_network_candidate() const { return !url.empty() || port > 0; }
    bool is_container_candidate() const {
        return docker_container.has_value() || k8s_pod.has_value();
    }

    // Check if this candidate is a direct detection (doesn't need active testing)
    // Direct Detection Criteria:
    // 1. claude_extension_installed - Explicitly installed by Claude Desktop
    // 2. config_declared - Explicitly configured in config file
    // 3. Rulepack evidence (source starts with "rulepack:") - Known MCP server pattern
    // Note: Actively confirmed servers (successful MCP protocol response) are also
    // considered direct detections, but that's handled separately in reporting
    bool is_direct_detection() const {
        for (const auto& e : evidence) {
            // Direct indicators
            if (e.type == "claude_extension_installed") return true;
            if (e.type == "config_declared") return true;

            // Rulepack detections are direct indicators
            if (e.source.find("rulepack:") == 0) return true;
        }
        return false;
    }

    void add_evidence(const Evidence& e) {
        evidence.push_back(e);
        // Recalculate confidence score (weighted average)
        recalculate_confidence();
    }

    void recalculate_confidence();
};

} // namespace kyros

#endif // KYROS_CANDIDATE_HPP
