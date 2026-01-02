#include <kyros/testing/testing_engine.hpp>

namespace kyros {

bool TestingEngine::is_valid_mcp_response(const nlohmann::json& response) const {
    // Check for required JSON-RPC fields
    if (!response.contains("jsonrpc") || response["jsonrpc"] != "2.0") {
        return false;
    }

    // Must have either "result" or "error"
    if (!response.contains("result") && !response.contains("error")) {
        return false;
    }

    // Must have "id" field
    if (!response.contains("id")) {
        return false;
    }

    return true;
}

nlohmann::json TestingEngine::create_initialize_request(int id) const {
    return nlohmann::json{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "initialize"},
        {"params", {
            {"protocolVersion", "2024-11-05"},
            {"capabilities", nlohmann::json::object()},
            {"clientInfo", {
                {"name", "Kyros"},
                {"version", "2.0.0"}
            }}
        }}
    };
}

} // namespace kyros
