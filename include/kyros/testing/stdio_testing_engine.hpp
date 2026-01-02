#ifndef KYROS_STDIO_TESTING_ENGINE_HPP
#define KYROS_STDIO_TESTING_ENGINE_HPP

#include <kyros/testing/testing_engine.hpp>
#include <kyros/platform/platform_adapter.hpp>

namespace kyros {

class StdioTestingEngine : public TestingEngine {
public:
    explicit StdioTestingEngine(std::shared_ptr<PlatformAdapter> platform);

    std::string name() const override { return "StdioTestingEngine"; }
    std::optional<MCPServer> test(const Candidate& candidate) override;

private:
    std::shared_ptr<PlatformAdapter> platform_;

    // Helper methods
    void extract_server_info(const nlohmann::json& response, MCPServer& server);
};

} // namespace kyros

#endif
