/**
 * Kyros ServerInterrogator Test Suite
 * Tests ServerInterrogator functionality using Google Test framework
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <kyros/scanner.hpp>
#include <kyros/testing/server_interrogator.hpp>
#include <kyros/mcp_server.hpp>
#include <nlohmann/json.hpp>

using ::testing::ElementsAre;
using ::testing::SizeIs;

// Test fixture for ServerInterrogator tests
class ServerInterrogatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        config.interrogate_enabled = true;
        config.max_tools = 100;
        config.max_resources = 100;
        config.max_prompts = 50;
    }

    kyros::InterrogationConfig config;
};

// ============================================================================
// Request Creation Tests
// ============================================================================

TEST_F(ServerInterrogatorTest, CreateToolsListRequest) {
    kyros::ServerInterrogator interrogator(config, nullptr, nullptr);
    auto request = interrogator.create_tools_list_request(1);

    EXPECT_EQ(request["jsonrpc"], "2.0");
    EXPECT_EQ(request["id"], 1);
    EXPECT_EQ(request["method"], "tools/list");
}

TEST_F(ServerInterrogatorTest, CreateResourcesListRequest) {
    kyros::ServerInterrogator interrogator(config, nullptr, nullptr);
    auto request = interrogator.create_resources_list_request(2);

    EXPECT_EQ(request["jsonrpc"], "2.0");
    EXPECT_EQ(request["id"], 2);
    EXPECT_EQ(request["method"], "resources/list");
}

TEST_F(ServerInterrogatorTest, CreateResourceTemplatesListRequest) {
    kyros::ServerInterrogator interrogator(config, nullptr, nullptr);
    auto request = interrogator.create_resource_templates_list_request(3);

    EXPECT_EQ(request["jsonrpc"], "2.0");
    EXPECT_EQ(request["id"], 3);
    EXPECT_EQ(request["method"], "resources/templates/list");
}

TEST_F(ServerInterrogatorTest, CreatePromptsListRequest) {
    kyros::ServerInterrogator interrogator(config, nullptr, nullptr);
    auto request = interrogator.create_prompts_list_request(4);

    EXPECT_EQ(request["jsonrpc"], "2.0");
    EXPECT_EQ(request["id"], 4);
    EXPECT_EQ(request["method"], "prompts/list");
}

// ============================================================================
// Tools Response Parsing Tests
// ============================================================================

TEST_F(ServerInterrogatorTest, ParseToolsResponse) {
    kyros::ServerInterrogator interrogator(config, nullptr, nullptr);
    kyros::MCPServer server;

    nlohmann::json tools_response = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"result", {
            {"tools", nlohmann::json::array({
                {
                    {"name", "read_file"},
                    {"description", "Read a file from the filesystem"},
                    {"inputSchema", {
                        {"type", "object"},
                        {"properties", {
                            {"path", {{"type", "string"}, {"description", "File path"}}},
                            {"encoding", {{"type", "string"}, {"description", "File encoding"}}}
                        }},
                        {"required", nlohmann::json::array({"path"})}
                    }}
                },
                {
                    {"name", "write_file"},
                    {"description", "Write content to a file"},
                    {"inputSchema", {
                        {"type", "object"},
                        {"properties", {
                            {"path", {{"type", "string"}}},
                            {"content", {{"type", "string"}}}
                        }},
                        {"required", nlohmann::json::array({"path", "content"})}
                    }}
                }
            })}
        }}
    };

    interrogator.parse_tools_response(tools_response, server);

    ASSERT_THAT(server.tools, SizeIs(2));

    // Verify first tool
    EXPECT_EQ(server.tools[0].name, "read_file");
    EXPECT_EQ(server.tools[0].description, "Read a file from the filesystem");
    EXPECT_THAT(server.tools[0].required_parameters, ElementsAre("path"));
    EXPECT_THAT(server.tools[0].optional_parameters, ElementsAre("encoding"));

    // Verify second tool
    EXPECT_EQ(server.tools[1].name, "write_file");
    EXPECT_THAT(server.tools[1].required_parameters, ElementsAre("path", "content"));
}

TEST_F(ServerInterrogatorTest, ParseToolsWithoutInputSchema) {
    kyros::ServerInterrogator interrogator(config, nullptr, nullptr);
    kyros::MCPServer server;

    nlohmann::json tools_response = {
        {"result", {
            {"tools", nlohmann::json::array({
                {
                    {"name", "simple_tool"},
                    {"description", "A simple tool"}
                    // No inputSchema
                }
            })}
        }}
    };

    interrogator.parse_tools_response(tools_response, server);

    ASSERT_THAT(server.tools, SizeIs(1));
    EXPECT_EQ(server.tools[0].name, "simple_tool");
    EXPECT_TRUE(server.tools[0].required_parameters.empty());
    EXPECT_TRUE(server.tools[0].optional_parameters.empty());
}

// ============================================================================
// Resources Response Parsing Tests
// ============================================================================

TEST_F(ServerInterrogatorTest, ParseResourcesResponse) {
    kyros::ServerInterrogator interrogator(config, nullptr, nullptr);
    kyros::MCPServer server;

    nlohmann::json resources_response = {
        {"jsonrpc", "2.0"},
        {"id", 2},
        {"result", {
            {"resources", nlohmann::json::array({
                {
                    {"uri", "file:///Users/test/file.txt"},
                    {"name", "Test File"},
                    {"description", "A test file"},
                    {"mimeType", "text/plain"}
                },
                {
                    {"uri", "file:///Users/test/data.json"},
                    {"name", "Data File"},
                    {"description", "JSON data"},
                    {"mimeType", "application/json"}
                }
            })}
        }}
    };

    interrogator.parse_resources_response(resources_response, server);

    ASSERT_THAT(server.resources, SizeIs(2));

    EXPECT_EQ(server.resources[0].uri, "file:///Users/test/file.txt");
    EXPECT_EQ(server.resources[0].name, "Test File");
    EXPECT_EQ(server.resources[0].mime_type, "text/plain");

    EXPECT_EQ(server.resources[1].uri, "file:///Users/test/data.json");
    EXPECT_EQ(server.resources[1].mime_type, "application/json");
}

// ============================================================================
// Resource Templates Response Parsing Tests
// ============================================================================

TEST_F(ServerInterrogatorTest, ParseResourceTemplatesResponse) {
    kyros::ServerInterrogator interrogator(config, nullptr, nullptr);
    kyros::MCPServer server;

    nlohmann::json templates_response = {
        {"jsonrpc", "2.0"},
        {"id", 3},
        {"result", {
            {"resourceTemplates", nlohmann::json::array({
                {
                    {"uriTemplate", "file:///{path}"},
                    {"name", "File Template"},
                    {"description", "Access any file by path"},
                    {"mimeType", "application/octet-stream"}
                },
                {
                    {"uriTemplate", "user:///{userId}/profile/{field}"},
                    {"name", "User Profile Template"},
                    {"description", "Access user profile fields"},
                    {"mimeType", "application/json"}
                }
            })}
        }}
    };

    interrogator.parse_resource_templates_response(templates_response, server);

    ASSERT_THAT(server.resource_templates, SizeIs(2));

    // Verify first template
    EXPECT_EQ(server.resource_templates[0].uri_template, "file:///{path}");
    EXPECT_EQ(server.resource_templates[0].name, "File Template");
    EXPECT_THAT(server.resource_templates[0].parameters, ElementsAre("path"));

    // Verify second template (multiple parameters)
    EXPECT_THAT(server.resource_templates[1].parameters, ElementsAre("userId", "field"));
}

// ============================================================================
// Prompts Response Parsing Tests
// ============================================================================

TEST_F(ServerInterrogatorTest, ParsePromptsResponse) {
    kyros::ServerInterrogator interrogator(config, nullptr, nullptr);
    kyros::MCPServer server;

    nlohmann::json prompts_response = {
        {"jsonrpc", "2.0"},
        {"id", 4},
        {"result", {
            {"prompts", nlohmann::json::array({
                {
                    {"name", "code_review"},
                    {"description", "Review code for issues"},
                    {"arguments", nlohmann::json::array({
                        {
                            {"name", "file"},
                            {"description", "File to review"},
                            {"required", true}
                        },
                        {
                            {"name", "severity"},
                            {"description", "Minimum severity"},
                            {"required", false}
                        }
                    })}
                }
            })}
        }}
    };

    interrogator.parse_prompts_response(prompts_response, server);

    ASSERT_THAT(server.prompts, SizeIs(1));
    EXPECT_EQ(server.prompts[0].name, "code_review");
    EXPECT_EQ(server.prompts[0].description, "Review code for issues");
    ASSERT_THAT(server.prompts[0].arguments, SizeIs(2));

    EXPECT_EQ(server.prompts[0].arguments[0].name, "file");
    EXPECT_TRUE(server.prompts[0].arguments[0].required);

    EXPECT_EQ(server.prompts[0].arguments[1].name, "severity");
    EXPECT_FALSE(server.prompts[0].arguments[1].required);
}

// ============================================================================
// Limit Enforcement Tests
// ============================================================================

TEST_F(ServerInterrogatorTest, EnforceToolsLimit) {
    config.max_tools = 2;
    kyros::ServerInterrogator interrogator(config, nullptr, nullptr);
    kyros::MCPServer server;

    nlohmann::json tools_response = {
        {"result", {
            {"tools", nlohmann::json::array({
                {{"name", "tool1"}, {"description", "Tool 1"}, {"inputSchema", nlohmann::json::object()}},
                {{"name", "tool2"}, {"description", "Tool 2"}, {"inputSchema", nlohmann::json::object()}},
                {{"name", "tool3"}, {"description", "Tool 3"}, {"inputSchema", nlohmann::json::object()}}
            })}
        }}
    };

    interrogator.parse_tools_response(tools_response, server);
    EXPECT_THAT(server.tools, SizeIs(2));
}

TEST_F(ServerInterrogatorTest, EnforceResourcesLimit) {
    config.max_resources = 1;
    kyros::ServerInterrogator interrogator(config, nullptr, nullptr);
    kyros::MCPServer server;

    nlohmann::json resources_response = {
        {"result", {
            {"resources", nlohmann::json::array({
                {{"uri", "file:///1"}, {"name", "R1"}},
                {{"uri", "file:///2"}, {"name", "R2"}}
            })}
        }}
    };

    interrogator.parse_resources_response(resources_response, server);
    EXPECT_THAT(server.resources, SizeIs(1));
}

// ============================================================================
// MCPServer Capability Tests
// ============================================================================

TEST_F(ServerInterrogatorTest, ServerCapabilityHelpers) {
    kyros::MCPServer server;

    // Initially no capabilities
    EXPECT_FALSE(server.has_tools());
    EXPECT_FALSE(server.has_resources());
    EXPECT_FALSE(server.has_prompts());

    // Add capabilities
    server.capabilities = nlohmann::json{
        {"tools", nlohmann::json::object()},
        {"resources", nlohmann::json::object()},
        {"prompts", nlohmann::json::object()}
    };

    EXPECT_TRUE(server.has_tools());
    EXPECT_TRUE(server.has_resources());
    EXPECT_TRUE(server.has_prompts());

    // Test with null capability values
    server.capabilities["tools"] = nullptr;
    EXPECT_FALSE(server.has_tools());
}

// ============================================================================
// InterrogationConfig Tests
// ============================================================================

TEST(InterrogationConfigTest, DefaultValues) {
    kyros::InterrogationConfig config;

    EXPECT_FALSE(config.interrogate_enabled);
    EXPECT_TRUE(config.get_tools);
    EXPECT_TRUE(config.get_resources);
    EXPECT_TRUE(config.get_prompts);
    EXPECT_TRUE(config.get_resource_templates);
    EXPECT_EQ(config.max_tools, 100);
    EXPECT_EQ(config.max_resources, 100);
    EXPECT_EQ(config.max_prompts, 50);
    EXPECT_EQ(config.timeout.count(), 5000);
}

// ============================================================================
// Scanner Basic Tests
// ============================================================================

TEST(ScannerTest, BasicConfiguration) {
    kyros::Scanner scanner;
    kyros::ScanConfig config;

    config.mode = kyros::ScanMode::PassiveOnly;
    config.passive_config.scan_configs = true;
    config.passive_config.scan_processes = false;
    config.passive_config.scan_network = false;

    // Should not crash
    SUCCEED();
}
