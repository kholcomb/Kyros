#ifndef KYROS_HTTP_TESTING_ENGINE_HPP
#define KYROS_HTTP_TESTING_ENGINE_HPP

#include <kyros/testing/testing_engine.hpp>
#include <kyros/http/http_client.hpp>

#include <memory>

namespace kyros {

class HttpTestingEngine : public TestingEngine {
public:
    explicit HttpTestingEngine(std::shared_ptr<HttpClient> http_client);

    std::string name() const override { return "HttpTestingEngine"; }
    std::optional<MCPServer> test(const Candidate& candidate) override;

private:
    std::shared_ptr<HttpClient> http_client_;

    // Helper methods
    void extract_server_info(const nlohmann::json& response, MCPServer& server);
    std::optional<MCPServer> try_sse_transport(const Candidate& candidate);
    std::string parse_sse_endpoint(const std::string& sse_body);
};

} // namespace kyros

#endif
