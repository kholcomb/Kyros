#include <kyros/detection/process_detection_engine.hpp>
#include <algorithm>
#include <cmath>

namespace kyros {

std::string ProcessDetectionEngine::name() const {
    return "ProcessDetectionEngine";
}

std::vector<Candidate> ProcessDetectionEngine::detect() {
    std::vector<Candidate> candidates;

    if (!platform_) {
        last_scan_process_count_ = 0;
        return candidates;
    }

    // Get all running processes
    std::vector<int> pids = platform_->get_process_list();
    last_scan_process_count_ = static_cast<int>(pids.size());

    // Check each process for MCP server indicators
    for (int pid : pids) {
        Candidate candidate;
        candidate.pid = pid;

        // Get process name and command line
        candidate.process_name = platform_->get_process_name(pid);
        std::string cmdline = platform_->get_command_line(pid);

        // Skip if we can't get basic info
        if (candidate.process_name.empty() && cmdline.empty()) {
            continue;
        }

        // Store command for reference
        if (!cmdline.empty()) {
            candidate.command = cmdline;
        }

        // Check various indicators
        check_parent_process(pid, candidate);
        check_file_descriptors(pid, candidate);
        check_environment(pid, candidate);

        // Only add candidates with at least some evidence
        if (!candidate.evidence.empty()) {
            candidates.push_back(candidate);
        }
    }

    return candidates;
}

void ProcessDetectionEngine::check_parent_process(int pid, Candidate& candidate) {
    if (!platform_) return;

    int parent_pid = platform_->get_parent_pid(pid);
    if (parent_pid <= 0) return;

    std::string parent_name = platform_->get_process_name(parent_pid);
    if (parent_name.empty()) return;

    // Known MCP client applications
    const std::vector<std::string> known_clients = {
        "Claude", "claude", "Claude.app",
        "Cursor", "cursor",
        "code", "Code", "Visual Studio Code",
        "windsurf", "Windsurf"
    };

    // Check if parent is a known MCP client
    for (const auto& client : known_clients) {
        if (parent_name.find(client) != std::string::npos) {
            Evidence evidence(
                "parent_process",
                "Parent process is MCP client: " + parent_name,
                0.7,  // Confidence score
                "",   // Source (empty)
                Evidence::Strength::Weak  // Weak evidence - too many false positives alone
            );
            candidate.add_evidence(evidence);
            break;
        }
    }
}

void ProcessDetectionEngine::check_file_descriptors(int pid, Candidate& candidate) {
    if (!platform_) return;

    // Check if process has bidirectional pipes (stdin + stdout both pipes)
    // This is a strong indicator of MCP stdio transport
    if (platform_->has_bidirectional_pipes(pid)) {
        Evidence evidence(
            "file_descriptors",
            "Process has bidirectional pipes (stdio transport)",
            0.6,  // Confidence score
            "",   // Source (empty)
            Evidence::Strength::Moderate  // Moderate - LSP/IPC also use pipes
        );
        candidate.add_evidence(evidence);
        candidate.transport_hint = TransportType::Stdio;
    }
}

void ProcessDetectionEngine::check_environment(int pid, Candidate& candidate) {
    if (!platform_) return;

    auto env = platform_->get_environment(pid);

    // Look for MCP-related environment variables
    const std::vector<std::string> mcp_env_vars = {
        "MCP_", "ANTHROPIC_", "CLAUDE_"
    };

    for (const auto& [key, value] : env) {
        for (const auto& prefix : mcp_env_vars) {
            if (key.find(prefix) == 0) {
                Evidence evidence(
                    "environment",
                    "Environment variable found: " + key,
                    0.5,  // Confidence score
                    "",   // Source (empty)
                    Evidence::Strength::Moderate  // Moderate - good corroborating evidence
                );
                candidate.add_evidence(evidence);
                break;
            }
        }
    }
}

} // namespace kyros
