#ifndef KYROS_TESTING_ENGINE_HPP
#define KYROS_TESTING_ENGINE_HPP

#include <kyros/candidate.hpp>
#include <kyros/mcp_server.hpp>
#include <nlohmann/json.hpp>
#include <chrono>
#include <optional>

namespace kyros {

class TestingEngine {
public:
    virtual ~TestingEngine() = default;

    virtual std::string name() const = 0;
    virtual std::optional<MCPServer> test(const Candidate& candidate) = 0;

    void set_timeout(std::chrono::milliseconds timeout) { timeout_ = timeout; }
    std::chrono::milliseconds timeout() const { return timeout_; }

protected:
    std::chrono::milliseconds timeout_{5000};

    bool is_valid_mcp_response(const nlohmann::json& response) const;
    nlohmann::json create_initialize_request(int id = 1) const;
};

} // namespace kyros

#endif
