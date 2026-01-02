#include <kyros/testing/server_interrogator.hpp>
#include <kyros/platform/process.hpp>

#include <iostream>

namespace kyros {

ServerInterrogator::ServerInterrogator(
    const InterrogationConfig& config,
    std::shared_ptr<PlatformAdapter> platform,
    std::shared_ptr<HttpClient> http_client)
    : config_(config), platform_(platform), http_client_(http_client) {}

void ServerInterrogator::interrogate(MCPServer& server) {
    server.interrogation_attempted = true;

    if (!config_.interrogate_enabled) {
        return;
    }

    auto start_time = std::chrono::steady_clock::now();

    try {
        // Create a communication function based on transport type
        std::function<nlohmann::json(const nlohmann::json&)> send_request;
        std::unique_ptr<Process> process;  // For stdio servers

        if (server.transport_type == TransportType::Stdio) {
            // Stdio transport: spawn process and communicate via pipes
            if (!platform_ || server.candidate.command.empty()) {
                server.interrogation_errors.push_back("Cannot interrogate stdio server: missing platform or command");
                server.interrogation_successful = false;
                return;
            }

            // Spawn the server process
            process = platform_->spawn_process_with_pipes(server.candidate.command);
            if (!process || !process->is_running()) {
                server.interrogation_errors.push_back("Failed to spawn process for interrogation");
                server.interrogation_successful = false;
                return;
            }

            // Create send_request lambda for stdio
            send_request = [&](const nlohmann::json& request) -> nlohmann::json {
                // Send request
                std::string request_str = request.dump() + "\n";
                process->write_stdin(request_str);

                // Read response with timeout
                std::string response_line = process->read_stdout_line(config_.timeout);

                // Parse and return
                return nlohmann::json::parse(response_line);
            };

        } else if (server.transport_type == TransportType::Http) {
            // HTTP transport: send POST requests
            if (!http_client_ || server.candidate.url.empty()) {
                server.interrogation_errors.push_back("Cannot interrogate HTTP server: missing HTTP client or URL");
                server.interrogation_successful = false;
                return;
            }

            // Create send_request lambda for HTTP
            send_request = [&](const nlohmann::json& request) -> nlohmann::json {
                std::string request_body = request.dump();
                auto response = http_client_->post(
                    server.candidate.url,
                    request_body,
                    {{"Content-Type", "application/json"}},
                    config_.timeout
                );

                if (response.status_code != 200) {
                    throw std::runtime_error("HTTP request failed with status " + std::to_string(response.status_code));
                }

                return nlohmann::json::parse(response.body);
            };

        } else {
            server.interrogation_errors.push_back("Unknown transport type");
            server.interrogation_successful = false;
            return;
        }

        // Perform interrogation based on config
        if (config_.get_tools && server.has_tools()) {
            interrogate_tools(server, send_request);
        }

        if (config_.get_resources && server.has_resources()) {
            interrogate_resources(server, send_request);
        }

        if (config_.get_resource_templates && server.has_resources()) {
            interrogate_resource_templates(server, send_request);
        }

        if (config_.get_prompts && server.has_prompts()) {
            interrogate_prompts(server, send_request);
        }

        // Clean up stdio process if needed
        if (process && process->is_running()) {
            process->terminate();
        }

    } catch (const std::exception& e) {
        server.interrogation_errors.push_back(std::string("Interrogation failed: ") + e.what());
    }

    auto end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    server.interrogation_time_seconds = elapsed.count();
    server.interrogation_successful = server.interrogation_errors.empty();
}

// Interrogation helper methods

void ServerInterrogator::interrogate_tools(
    MCPServer& server,
    const std::function<nlohmann::json(const nlohmann::json&)>& send_request) {
    try {
        auto request = create_tools_list_request(1);
        auto response = send_request(request);
        parse_tools_response(response, server);
    } catch (const std::exception& e) {
        server.interrogation_errors.push_back(std::string("Tools interrogation failed: ") + e.what());
    }
}

void ServerInterrogator::interrogate_resources(
    MCPServer& server,
    const std::function<nlohmann::json(const nlohmann::json&)>& send_request) {
    try {
        auto request = create_resources_list_request(2);
        auto response = send_request(request);
        parse_resources_response(response, server);
    } catch (const std::exception& e) {
        server.interrogation_errors.push_back(std::string("Resources interrogation failed: ") + e.what());
    }
}

void ServerInterrogator::interrogate_resource_templates(
    MCPServer& server,
    const std::function<nlohmann::json(const nlohmann::json&)>& send_request) {
    try {
        auto request = create_resource_templates_list_request(3);
        auto response = send_request(request);
        parse_resource_templates_response(response, server);
    } catch (const std::exception& e) {
        server.interrogation_errors.push_back(std::string("Resource templates interrogation failed: ") + e.what());
    }
}

void ServerInterrogator::interrogate_prompts(
    MCPServer& server,
    const std::function<nlohmann::json(const nlohmann::json&)>& send_request) {
    try {
        auto request = create_prompts_list_request(4);
        auto response = send_request(request);
        parse_prompts_response(response, server);
    } catch (const std::exception& e) {
        server.interrogation_errors.push_back(std::string("Prompts interrogation failed: ") + e.what());
    }
}

// Request creation helpers

nlohmann::json ServerInterrogator::create_tools_list_request(int id) const {
    return nlohmann::json{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "tools/list"},
        {"params", nlohmann::json::object()}
    };
}

nlohmann::json ServerInterrogator::create_resources_list_request(int id) const {
    return nlohmann::json{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "resources/list"},
        {"params", nlohmann::json::object()}
    };
}

nlohmann::json ServerInterrogator::create_resource_templates_list_request(int id) const {
    return nlohmann::json{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "resources/templates/list"},
        {"params", nlohmann::json::object()}
    };
}

nlohmann::json ServerInterrogator::create_prompts_list_request(int id) const {
    return nlohmann::json{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "prompts/list"},
        {"params", nlohmann::json::object()}
    };
}

// Response parsing helpers

void ServerInterrogator::parse_tools_response(const nlohmann::json& response, MCPServer& server) {
    if (!response.contains("result")) {
        return;
    }

    const auto& result = response["result"];
    if (!result.contains("tools") || !result["tools"].is_array()) {
        return;
    }

    int count = 0;
    for (const auto& tool_json : result["tools"]) {
        if (count >= config_.max_tools) {
            break;
        }

        ToolDefinition tool;

        if (tool_json.contains("name") && tool_json["name"].is_string()) {
            tool.name = tool_json["name"].get<std::string>();
        }

        if (tool_json.contains("description") && tool_json["description"].is_string()) {
            tool.description = tool_json["description"].get<std::string>();
        }

        if (tool_json.contains("inputSchema") && tool_json["inputSchema"].is_object()) {
            tool.input_schema = tool_json["inputSchema"];

            // Extract required and optional parameters
            if (tool.input_schema.contains("required") && tool.input_schema["required"].is_array()) {
                for (const auto& req : tool.input_schema["required"]) {
                    if (req.is_string()) {
                        tool.required_parameters.push_back(req.get<std::string>());
                    }
                }
            }

            if (tool.input_schema.contains("properties") && tool.input_schema["properties"].is_object()) {
                for (auto it = tool.input_schema["properties"].begin(); it != tool.input_schema["properties"].end(); ++it) {
                    std::string param_name = it.key();
                    // If not in required, it's optional
                    if (std::find(tool.required_parameters.begin(), tool.required_parameters.end(), param_name)
                        == tool.required_parameters.end()) {
                        tool.optional_parameters.push_back(param_name);
                    }
                }
            }
        }

        server.tools.push_back(std::move(tool));
        count++;
    }
}

void ServerInterrogator::parse_resources_response(const nlohmann::json& response, MCPServer& server) {
    if (!response.contains("result")) {
        return;
    }

    const auto& result = response["result"];
    if (!result.contains("resources") || !result["resources"].is_array()) {
        return;
    }

    int count = 0;
    for (const auto& resource_json : result["resources"]) {
        if (count >= config_.max_resources) {
            break;
        }

        ResourceDefinition resource;

        if (resource_json.contains("uri") && resource_json["uri"].is_string()) {
            resource.uri = resource_json["uri"].get<std::string>();
        }

        if (resource_json.contains("name") && resource_json["name"].is_string()) {
            resource.name = resource_json["name"].get<std::string>();
        }

        if (resource_json.contains("description") && resource_json["description"].is_string()) {
            resource.description = resource_json["description"].get<std::string>();
        }

        if (resource_json.contains("mimeType") && resource_json["mimeType"].is_string()) {
            resource.mime_type = resource_json["mimeType"].get<std::string>();
        }

        server.resources.push_back(std::move(resource));
        count++;
    }
}

void ServerInterrogator::parse_resource_templates_response(const nlohmann::json& response, MCPServer& server) {
    if (!response.contains("result")) {
        return;
    }

    const auto& result = response["result"];
    if (!result.contains("resourceTemplates") || !result["resourceTemplates"].is_array()) {
        return;
    }

    int count = 0;
    for (const auto& template_json : result["resourceTemplates"]) {
        if (count >= config_.max_resources) {
            break;
        }

        ResourceTemplate resource_template;

        if (template_json.contains("uriTemplate") && template_json["uriTemplate"].is_string()) {
            resource_template.uri_template = template_json["uriTemplate"].get<std::string>();
        }

        if (template_json.contains("name") && template_json["name"].is_string()) {
            resource_template.name = template_json["name"].get<std::string>();
        }

        if (template_json.contains("description") && template_json["description"].is_string()) {
            resource_template.description = template_json["description"].get<std::string>();
        }

        if (template_json.contains("mimeType") && template_json["mimeType"].is_string()) {
            resource_template.mime_type = template_json["mimeType"].get<std::string>();
        }

        // Extract template parameters from uriTemplate (simple extraction)
        // MCP URI templates use {param} syntax
        std::string uri_template = resource_template.uri_template;
        size_t pos = 0;
        while ((pos = uri_template.find('{', pos)) != std::string::npos) {
            size_t end_pos = uri_template.find('}', pos);
            if (end_pos != std::string::npos) {
                std::string param = uri_template.substr(pos + 1, end_pos - pos - 1);
                resource_template.parameters.push_back(param);
                pos = end_pos + 1;
            } else {
                break;
            }
        }

        server.resource_templates.push_back(std::move(resource_template));
        count++;
    }
}

void ServerInterrogator::parse_prompts_response(const nlohmann::json& response, MCPServer& server) {
    if (!response.contains("result")) {
        return;
    }

    const auto& result = response["result"];
    if (!result.contains("prompts") || !result["prompts"].is_array()) {
        return;
    }

    int count = 0;
    for (const auto& prompt_json : result["prompts"]) {
        if (count >= config_.max_prompts) {
            break;
        }

        PromptDefinition prompt;

        if (prompt_json.contains("name") && prompt_json["name"].is_string()) {
            prompt.name = prompt_json["name"].get<std::string>();
        }

        if (prompt_json.contains("description") && prompt_json["description"].is_string()) {
            prompt.description = prompt_json["description"].get<std::string>();
        }

        if (prompt_json.contains("arguments") && prompt_json["arguments"].is_array()) {
            for (const auto& arg_json : prompt_json["arguments"]) {
                PromptArgument arg;

                if (arg_json.contains("name") && arg_json["name"].is_string()) {
                    arg.name = arg_json["name"].get<std::string>();
                }

                if (arg_json.contains("description") && arg_json["description"].is_string()) {
                    arg.description = arg_json["description"].get<std::string>();
                }

                if (arg_json.contains("required") && arg_json["required"].is_boolean()) {
                    arg.required = arg_json["required"].get<bool>();
                }

                prompt.arguments.push_back(std::move(arg));
            }
        }

        server.prompts.push_back(std::move(prompt));
        count++;
    }
}

} // namespace kyros
