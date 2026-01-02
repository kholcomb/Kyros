#include <kyros/testing/http_testing_engine.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iostream>
#include <sstream>

namespace kyros {

HttpTestingEngine::HttpTestingEngine(std::shared_ptr<HttpClient> http_client)
    : http_client_(http_client) {}

std::optional<MCPServer> HttpTestingEngine::test(const Candidate& candidate) {
    // Check if candidate has a URL
    if (candidate.url.empty()) {
        return std::nullopt;
    }

    // Only test HTTP transport candidates
    if (candidate.transport_hint != TransportType::Http &&
        candidate.transport_hint != TransportType::Unknown) {
        return std::nullopt;
    }

    if (!http_client_) {
        return std::nullopt;
    }

    // First, try SSE-based MCP transport
    auto sse_result = try_sse_transport(candidate);
    if (sse_result.has_value()) {
        return sse_result;
    }

    // Fall back to trying direct HTTP POST to common MCP paths
    std::vector<std::string> paths_to_try = {"", "/messages", "/rpc"};

    for (const auto& path : paths_to_try) {
        std::string test_url = candidate.url + path;

        try {
            // Create the MCP initialize request
            auto request = create_initialize_request(1);
            std::string request_body = request.dump();

            // Send HTTP POST request to the server
            auto response = http_client_->post(test_url, request_body, {}, timeout_);

            // Check HTTP status code - accept 200 OK or auth-related responses
            bool is_success = (response.status_code == 200);
            bool is_auth_challenge = (response.status_code == 401 || response.status_code == 403);

            if (!is_success && !is_auth_challenge) {
                continue;  // Try next path
            }

            // Parse the JSON response
            nlohmann::json json_response;
            bool is_json = false;
            try {
                json_response = nlohmann::json::parse(response.body);
                is_json = true;
            } catch (const nlohmann::json::parse_error& e) {
                // Not JSON - check if it's an auth challenge with MCP keywords
                if (is_auth_challenge) {
                    std::string body_lower = response.body;
                    std::transform(body_lower.begin(), body_lower.end(), body_lower.begin(), ::tolower);

                    // Check for MCP-related keywords in auth responses
                    if (body_lower.find("authentication") != std::string::npos ||
                        body_lower.find("unauthorized") != std::string::npos ||
                        body_lower.find("session") != std::string::npos ||
                        body_lower.find("mcp") != std::string::npos) {
                        // Auth challenge with MCP keywords - this is an indicator!
                        // Continue to create server object
                    } else {
                        continue;  // Not MCP-related
                    }
                } else {
                    continue;  // Not JSON and not an auth challenge
                }
            }

            // If JSON, validate it's a proper JSON-RPC 2.0 response
            // (accepts both result and error responses - both are indicators)
            if (is_json && !is_valid_mcp_response(json_response)) {
                continue;  // Not valid JSON-RPC 2.0
            }

            // If we got here, we found a valid MCP indicator!
            // - Valid JSON-RPC 2.0 response (success or error)
            // - Or auth challenge with MCP-related keywords

            // Success! Create MCPServer object from successful response
            MCPServer server;
            server.candidate = candidate;
            server.candidate.url = test_url;  // Update URL to the successful path
            server.transport_type = TransportType::Http;
            server.discovered_at = std::chrono::system_clock::now();

            // Extract server information from the initialize response (if JSON)
            if (is_json) {
                extract_server_info(json_response, server);
            }
            // For auth challenges without JSON-RPC, we still confirm the server exists
            // but won't have detailed server info

            return server;

        } catch (const std::exception& e) {
            // Continue to next path on error
            continue;
        }
    }

    // All paths failed
    return std::nullopt;
}

std::optional<MCPServer> HttpTestingEngine::try_sse_transport(const Candidate& candidate) {
    // Try SSE endpoint
    std::string sse_url = candidate.url + "/sse";

    try {
        // Set Accept header for SSE
        std::map<std::string, std::string> headers;
        headers["Accept"] = "text/event-stream";

        // GET request to /sse endpoint
        auto response = http_client_->get(sse_url, headers, timeout_);

        // Check for auth challenge on SSE endpoint first
        if (response.status_code == 401 || response.status_code == 403) {
            // Auth-protected SSE endpoint - check for MCP keywords
            std::string body_lower = response.body;
            std::transform(body_lower.begin(), body_lower.end(), body_lower.begin(), ::tolower);

            if (body_lower.find("authentication") != std::string::npos ||
                body_lower.find("unauthorized") != std::string::npos ||
                body_lower.find("session") != std::string::npos ||
                body_lower.find("token") != std::string::npos ||
                body_lower.find("mcp") != std::string::npos) {
                // Found auth-protected SSE endpoint with MCP keywords!
                MCPServer server;
                server.candidate = candidate;
                server.candidate.url = sse_url;
                server.transport_type = TransportType::Http;
                server.discovered_at = std::chrono::system_clock::now();
                return server;
            }
            return std::nullopt;
        }

        // Must be HTTP 200 for successful SSE connection
        if (response.status_code != 200) {
            return std::nullopt;
        }

        // Check if Content-Type is text/event-stream
        if (response.headers.find("content-type") != response.headers.end()) {
            const auto& content_type = response.headers.at("content-type");
            if (content_type.find("text/event-stream") == std::string::npos) {
                return std::nullopt;
            }
        } else {
            return std::nullopt;
        }

        // Parse SSE response to extract endpoint
        std::string endpoint_path = parse_sse_endpoint(response.body);
        if (endpoint_path.empty()) {
            return std::nullopt;
        }

        // Build full URL for messages endpoint
        std::string messages_url = candidate.url + endpoint_path;

        // Now test the messages endpoint with MCP initialize
        auto request = create_initialize_request(1);
        std::string request_body = request.dump();

        auto messages_response = http_client_->post(messages_url, request_body, {}, timeout_);

        // Accept 200 OK or auth challenges
        if (messages_response.status_code != 200 &&
            messages_response.status_code != 401 &&
            messages_response.status_code != 403) {
            return std::nullopt;
        }

        // Parse the JSON response
        nlohmann::json json_response;
        try {
            json_response = nlohmann::json::parse(messages_response.body);
        } catch (const nlohmann::json::parse_error& e) {
            return std::nullopt;
        }

        // Validate it's a proper JSON-RPC 2.0 response (accepts both result and error)
        if (!is_valid_mcp_response(json_response)) {
            return std::nullopt;
        }

        // Any valid JSON-RPC 2.0 response is an indicator (success or error)

        // Success! Create MCPServer object
        MCPServer server;
        server.candidate = candidate;
        server.candidate.url = messages_url;  // Update URL to the messages endpoint
        server.transport_type = TransportType::Http;
        server.discovered_at = std::chrono::system_clock::now();

        // Extract server information from the initialize response
        extract_server_info(json_response, server);

        return server;

    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

std::string HttpTestingEngine::parse_sse_endpoint(const std::string& sse_body) {
    // Parse SSE format to extract endpoint
    // Expected format:
    // event: endpoint
    // data: /messages/?session_id=...

    std::istringstream stream(sse_body);
    std::string line;
    std::string endpoint;
    bool found_endpoint_event = false;

    while (std::getline(stream, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) {
            continue;
        }

        // Check for "event: endpoint"
        if (line.find("event:") == 0) {
            std::string event_type = line.substr(6);
            event_type.erase(0, event_type.find_first_not_of(" \t"));
            if (event_type == "endpoint") {
                found_endpoint_event = true;
            }
        }

        // Check for "data: ..." when we've seen the endpoint event
        if (found_endpoint_event && line.find("data:") == 0) {
            endpoint = line.substr(5);
            endpoint.erase(0, endpoint.find_first_not_of(" \t"));
            break;
        }
    }

    return endpoint;
}

void HttpTestingEngine::extract_server_info(const nlohmann::json& response,
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
