#ifndef KYROS_SERVER_INTERROGATOR_HPP
#define KYROS_SERVER_INTERROGATOR_HPP

#include <kyros/mcp_server.hpp>
#include <kyros/config.hpp>
#include <kyros/platform/platform_adapter.hpp>
#include <kyros/http/http_client.hpp>

#include <memory>
#include <nlohmann/json.hpp>

namespace kyros {

class ServerInterrogator {
public:
    ServerInterrogator(
        const InterrogationConfig& config,
        std::shared_ptr<PlatformAdapter> platform,
        std::shared_ptr<HttpClient> http_client
    );

    void interrogate(MCPServer& server);

    // Request creation helpers (public for testing)
    nlohmann::json create_tools_list_request(int id) const;
    nlohmann::json create_resources_list_request(int id) const;
    nlohmann::json create_resource_templates_list_request(int id) const;
    nlohmann::json create_prompts_list_request(int id) const;

    // Response parsing helpers (public for testing)
    void parse_tools_response(const nlohmann::json& response, MCPServer& server);
    void parse_resources_response(const nlohmann::json& response, MCPServer& server);
    void parse_resource_templates_response(const nlohmann::json& response, MCPServer& server);
    void parse_prompts_response(const nlohmann::json& response, MCPServer& server);

private:
    InterrogationConfig config_;
    std::shared_ptr<PlatformAdapter> platform_;
    std::shared_ptr<HttpClient> http_client_;

    // Helper methods for interrogation
    void interrogate_tools(MCPServer& server, const std::function<nlohmann::json(const nlohmann::json&)>& send_request);
    void interrogate_resources(MCPServer& server, const std::function<nlohmann::json(const nlohmann::json&)>& send_request);
    void interrogate_resource_templates(MCPServer& server, const std::function<nlohmann::json(const nlohmann::json&)>& send_request);
    void interrogate_prompts(MCPServer& server, const std::function<nlohmann::json(const nlohmann::json&)>& send_request);
};

} // namespace kyros

#endif
