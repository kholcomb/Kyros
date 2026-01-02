#include <kyros/detection/network_detection_engine.hpp>

namespace kyros {

std::string NetworkDetectionEngine::name() const {
    return "NetworkDetectionEngine";
}

std::vector<Candidate> NetworkDetectionEngine::detect() {
    std::vector<Candidate> candidates;

    if (!platform_) {
        last_scan_socket_count_ = 0;
        return candidates;
    }

    // Get all listening sockets
    auto listeners = platform_->get_listening_sockets();
    last_scan_socket_count_ = static_cast<int>(listeners.size());

    // Create candidate for each listener
    for (const auto& listener : listeners) {
        Candidate candidate;
        candidate.pid = listener.pid;

        // Build URL for HTTP transport
        // Default to http:// for local addresses
        std::string protocol = "http";
        std::string host = listener.address;

        // If address is 0.0.0.0 or ::, prefer localhost
        if (host == "0.0.0.0" || host == "::") {
            host = "127.0.0.1";
        }

        // IPv6 addresses must be wrapped in brackets for URLs
        bool is_ipv6 = (host.find(':') != std::string::npos);
        if (is_ipv6) {
            host = "[" + host + "]";
        }

        candidate.url = protocol + "://" + host + ":" + std::to_string(listener.port);
        candidate.transport_hint = TransportType::Http;

        // Get process info if available
        if (listener.pid > 0) {
            candidate.process_name = platform_->get_process_name(listener.pid);
            candidate.command = platform_->get_command_line(listener.pid);
        }

        // Add evidence
        Evidence evidence;
        evidence.type = "network_listener";
        evidence.description = "Process listening on " +
                             listener.address + ":" + std::to_string(listener.port) +
                             " (" + listener.protocol + ")";

        // Differentiate by protocol - UDP is very unlikely to be MCP (used for discovery/streaming)
        // MCP typically uses stdio or HTTP/SSE (which would be TCP)
        if (listener.protocol == "udp") {
            evidence.confidence = 0.05;  // Very low - UDP rarely used for MCP
        } else {
            evidence.confidence = 0.10;  // Low but higher than UDP - TCP could be HTTP transport
        }

        candidate.add_evidence(evidence);

        // Note: localhost binding is common for many services and doesn't strongly indicate MCP
        // Removed localhost_only confidence boost to avoid false positives

        candidates.push_back(candidate);
    }

    return candidates;
}

} // namespace kyros
