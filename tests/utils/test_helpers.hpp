/**
 * Test Helpers for Kyros Test Suite
 * Common utilities and helper functions for tests
 */

#pragma once

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <kyros/mcp_server.hpp>
#include <kyros/evidence.hpp>
#include <kyros/candidate.hpp>
#include <kyros/types.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

namespace kyros::test {

/**
 * Create a sample Evidence for testing
 */
inline Evidence create_test_evidence(
    const std::string& type,
    const std::string& description,
    double confidence = 0.8,
    const std::string& source = "test")
{
    return Evidence(type, description, confidence, source);
}

/**
 * Create a sample Candidate for testing
 */
inline Candidate create_test_candidate(
    const std::string& process_name = "node",
    int pid = 12345)
{
    Candidate candidate;
    candidate.process_name = process_name;
    candidate.pid = pid;
    candidate.command = "/usr/bin/" + process_name + " server.js";
    return candidate;
}

/**
 * Create a Candidate with evidence
 */
inline Candidate create_candidate_with_evidence(
    const std::string& process_name,
    int pid,
    const std::vector<Evidence>& evidence_list)
{
    Candidate candidate = create_test_candidate(process_name, pid);
    for (const auto& ev : evidence_list) {
        candidate.add_evidence(ev);
    }
    return candidate;
}

/**
 * Create a sample ToolDefinition for testing
 */
inline ToolDefinition create_test_tool(
    const std::string& name,
    const std::vector<std::string>& required_params = {},
    const std::vector<std::string>& optional_params = {})
{
    ToolDefinition tool;
    tool.name = name;
    tool.description = "Test tool: " + name;
    tool.required_parameters = required_params;
    tool.optional_parameters = optional_params;

    // Create a basic input schema
    tool.input_schema = nlohmann::json{
        {"type", "object"},
        {"properties", nlohmann::json::object()}
    };

    return tool;
}

/**
 * Create a sample ResourceDefinition for testing
 */
inline ResourceDefinition create_test_resource(
    const std::string& uri,
    const std::string& name = "Test Resource",
    const std::string& mime_type = "text/plain")
{
    ResourceDefinition resource;
    resource.uri = uri;
    resource.name = name;
    resource.mime_type = mime_type;
    resource.description = "Test resource";
    return resource;
}

/**
 * Create a sample ResourceTemplate for testing
 */
inline ResourceTemplate create_test_template(
    const std::string& uri_template,
    const std::vector<std::string>& parameters)
{
    ResourceTemplate templ;
    templ.uri_template = uri_template;
    templ.name = "Test Template";
    templ.parameters = parameters;
    templ.mime_type = "application/json";
    return templ;
}

/**
 * Create a sample PromptDefinition for testing
 */
inline PromptDefinition create_test_prompt(
    const std::string& name,
    const std::vector<std::pair<std::string, bool>>& args = {})
{
    PromptDefinition prompt;
    prompt.name = name;
    prompt.description = "Test prompt: " + name;

    for (const auto& [arg_name, required] : args) {
        PromptArgument arg;
        arg.name = arg_name;
        arg.required = required;
        arg.description = "Argument: " + arg_name;
        prompt.arguments.push_back(arg);
    }

    return prompt;
}

/**
 * Create a sample MCPServer for testing
 */
inline MCPServer create_test_server(
    const std::string& server_name = "test-server",
    TransportType transport = TransportType::Stdio)
{
    MCPServer server;
    server.server_name = server_name;
    server.server_version = "1.0.0";
    server.protocol_version = "2024-11-05";
    server.transport_type = transport;
    server.capabilities = nlohmann::json{
        {"tools", nlohmann::json::object()},
        {"resources", nlohmann::json::object()}
    };
    return server;
}

/**
 * Create a valid JSON-RPC request
 */
inline nlohmann::json create_jsonrpc_request(
    const std::string& method,
    int id = 1,
    const nlohmann::json& params = nullptr)
{
    nlohmann::json request = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", method}
    };

    if (!params.is_null()) {
        request["params"] = params;
    }

    return request;
}

/**
 * Create a valid JSON-RPC response
 */
inline nlohmann::json create_jsonrpc_response(
    int id,
    const nlohmann::json& result)
{
    return {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"result", result}
    };
}

/**
 * Create a JSON-RPC error response
 */
inline nlohmann::json create_jsonrpc_error(
    int id,
    int error_code,
    const std::string& error_message)
{
    return {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"error", {
            {"code", error_code},
            {"message", error_message}
        }}
    };
}

/**
 * Check if two JSON objects are equal (useful for comparing complex structures)
 */
inline void expect_json_eq(const nlohmann::json& expected, const nlohmann::json& actual) {
    EXPECT_EQ(expected.dump(), actual.dump())
        << "Expected: " << expected.dump(2) << "\n"
        << "Actual:   " << actual.dump(2);
}

/**
 * Temporary file helper for testing
 */
class TempFile {
public:
    explicit TempFile(const std::string& content = "")
        : path_("/tmp/kyros_test_" + std::to_string(counter_++))
    {
        if (!content.empty()) {
            write(content);
        }
    }

    ~TempFile() {
        std::remove(path_.c_str());
    }

    const std::string& path() const { return path_; }

    void write(const std::string& content) {
        std::ofstream file(path_);
        file << content;
    }

    std::string read() {
        std::ifstream file(path_);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

private:
    std::string path_;
    static inline int counter_ = 0;
};

} // namespace kyros::test
