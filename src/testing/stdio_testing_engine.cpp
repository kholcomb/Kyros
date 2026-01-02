#include <kyros/testing/stdio_testing_engine.hpp>
#include <kyros/testing/protocol_detector.hpp>
#include <kyros/platform/process.hpp>

#include <chrono>
#include <iostream>

namespace kyros {

StdioTestingEngine::StdioTestingEngine(std::shared_ptr<PlatformAdapter> platform)
    : platform_(platform) {}

std::optional<MCPServer> StdioTestingEngine::test(const Candidate& candidate) {
    // Check if candidate has a command to execute
    if (candidate.command.empty()) {
        return std::nullopt;
    }

    // Only test stdio transport candidates
    if (candidate.transport_hint != TransportType::Stdio &&
        candidate.transport_hint != TransportType::Unknown) {
        return std::nullopt;
    }

    if (!platform_) {
        return std::nullopt;
    }

    // PHASE 3: Passive protocol detection (before spawning)
    // Skip obvious non-MCP processes to save time and avoid false positives
    ProtocolDetector detector;
    auto passive_signature = detector.detect_from_process_info(candidate);

    // If confirmed NOT MCP (Chromium IPC or LSP), skip active testing
    if (passive_signature.type == ProtocolType::ChromiumIPC ||
        passive_signature.type == ProtocolType::LSP) {
        // Skip active testing for known non-MCP protocols
        return std::nullopt;
    }

    std::unique_ptr<Process> process;

    try {
        // Spawn the MCP server process with pipes
        process = platform_->spawn_process_with_pipes(candidate.command);

        if (!process || !process->is_running()) {
            return std::nullopt;
        }

        // PHASE 3: Active protocol detection (optional - after spawning)
        // This is a defense-in-depth measure for ambiguous cases
        // For now, we rely on the MCP test itself as the active detector
        // Future enhancement: Could probe protocol before full MCP handshake

        // Create the MCP initialize request
        auto request = create_initialize_request(1);
        std::string request_str = request.dump() + "\n";

        // Send the initialize request to the server's stdin
        process->write_stdin(request_str);

        // Read the response from stdout (with timeout)
        std::string response_line = process->read_stdout_line(timeout_);

        // Parse the JSON response
        nlohmann::json response;
        try {
            response = nlohmann::json::parse(response_line);
        } catch (const nlohmann::json::parse_error& e) {
            // Invalid JSON response - not a valid MCP server
            process->terminate();
            return std::nullopt;
        }

        // Validate it's a proper JSON-RPC 2.0 response
        // (accepts both result and error - both are MCP indicators)
        if (!is_valid_mcp_response(response)) {
            process->terminate();
            return std::nullopt;
        }

        // Any valid JSON-RPC 2.0 response is an indicator (success or error)

        // Create MCPServer object from successful response
        MCPServer server;
        server.candidate = candidate;
        server.transport_type = TransportType::Stdio;
        server.discovered_at = std::chrono::system_clock::now();

        // Extract server information from the initialize response
        extract_server_info(response, server);

        // Terminate the process (we're done testing)
        process->terminate();

        return server;

    } catch (const std::exception& e) {
        // Handle any errors during testing
        std::cerr << "Error testing candidate " << candidate.command << ": "
                  << e.what() << std::endl;

        // Clean up process if it exists
        if (process && process->is_running()) {
            process->terminate();
        }

        return std::nullopt;
    }
}

void StdioTestingEngine::extract_server_info(const nlohmann::json& response,
                                             MCPServer& server) {
    if (!response.contains("result")) {
        return;
    }

    const auto& result = response["result"];

    // Extract protocol version
    if (result.contains("protocolVersion") && result["protocolVersion"].is_string()) {
        server.protocol_version = result["protocolVersion"].get<std::string>();
    }

    // Extract server info
    if (result.contains("serverInfo") && result["serverInfo"].is_object()) {
        const auto& server_info = result["serverInfo"];

        if (server_info.contains("name") && server_info["name"].is_string()) {
            server.server_name = server_info["name"].get<std::string>();
        }

        if (server_info.contains("version") && server_info["version"].is_string()) {
            server.server_version = server_info["version"].get<std::string>();
        }
    }

    // Extract capabilities
    if (result.contains("capabilities") && result["capabilities"].is_object()) {
        server.capabilities = result["capabilities"];
    }
}

} // namespace kyros
