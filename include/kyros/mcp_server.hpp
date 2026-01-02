#ifndef KYROS_MCP_SERVER_HPP
#define KYROS_MCP_SERVER_HPP

#include <kyros/candidate.hpp>
#include <kyros/types.hpp>

#include <nlohmann/json.hpp>

namespace kyros {

// Tool definition from tools/list
struct ToolDefinition {
    std::string name;
    std::string description;
    nlohmann::json input_schema;

    // Derived information
    std::vector<std::string> required_parameters;
    std::vector<std::string> optional_parameters;
};

// Resource definition from resources/list
struct ResourceDefinition {
    std::string uri;
    std::string name;
    std::string description;
    std::string mime_type;
};

// Resource template from resources/templates/list
struct ResourceTemplate {
    std::string uri_template;
    std::string name;
    std::string description;
    std::string mime_type;
    std::vector<std::string> parameters;
};

// Prompt argument
struct PromptArgument {
    std::string name;
    std::string type;
    std::string description;
    bool required = false;
};

// Prompt definition from prompts/list
struct PromptDefinition {
    std::string name;
    std::string description;
    std::vector<PromptArgument> arguments;
};

/**
 * A confirmed MCP server (passed protocol test)
 */
struct MCPServer {
    // Original candidate information
    Candidate candidate;

    // Basic server information (from initialize response)
    std::string server_name;
    std::string server_version;
    std::string protocol_version;
    nlohmann::json capabilities;
    TransportType transport_type = TransportType::Unknown;

    // Interrogation results (empty if not interrogated)
    std::vector<ToolDefinition> tools;
    std::vector<ResourceDefinition> resources;
    std::vector<ResourceTemplate> resource_templates;
    std::vector<PromptDefinition> prompts;

    // Interrogation metadata
    bool interrogation_attempted = false;
    bool interrogation_successful = false;
    std::vector<std::string> interrogation_errors;
    double interrogation_time_seconds = 0.0;

    // Discovery metadata
    Timestamp discovered_at;

    // Helpers
    bool has_tools() const {
        return capabilities.contains("tools") && !capabilities["tools"].is_null();
    }

    bool has_resources() const {
        return capabilities.contains("resources") && !capabilities["resources"].is_null();
    }

    bool has_prompts() const {
        return capabilities.contains("prompts") && !capabilities["prompts"].is_null();
    }

    std::string endpoint() const {
        if (!candidate.url.empty()) {
            return candidate.url;
        } else if (candidate.pid > 0) {
            return "pid:" + std::to_string(candidate.pid);
        }
        return "unknown";
    }
};

} // namespace kyros

#endif // KYROS_MCP_SERVER_HPP
