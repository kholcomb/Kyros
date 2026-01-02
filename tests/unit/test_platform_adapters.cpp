/**
 * Platform Adapters Test Suite
 * Tests platform abstraction layer
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <kyros/platform/platform_adapter.hpp>
#include "../mocks/mock_platform_adapter.hpp"
#include "test_helpers.hpp"

using namespace kyros;
using namespace kyros::test;
using ::testing::Return;
using ::testing::SizeIs;
using ::testing::Contains;

// ============================================================================
// Mock Platform Adapter Tests
// ============================================================================

TEST(MockPlatformAdapterTest, GetProcessList) {
    MockPlatformAdapter adapter;

    std::vector<int> test_pids = {100, 200, 300};
    adapter.set_process_list(test_pids);

    auto pids = adapter.get_process_list();

    ASSERT_THAT(pids, SizeIs(3));
    EXPECT_THAT(pids, Contains(100));
    EXPECT_THAT(pids, Contains(200));
    EXPECT_THAT(pids, Contains(300));
}

TEST(MockPlatformAdapterTest, GetCommandLine) {
    MockPlatformAdapter adapter;

    EXPECT_CALL(adapter, get_command_line(12345))
        .WillOnce(Return("/usr/bin/node /app/server.js"));

    auto cmdline = adapter.get_command_line(12345);

    EXPECT_EQ(cmdline, "/usr/bin/node /app/server.js");
}

TEST(MockPlatformAdapterTest, GetProcessName) {
    MockPlatformAdapter adapter;

    EXPECT_CALL(adapter, get_process_name(100))
        .WillOnce(Return("node"));

    auto name = adapter.get_process_name(100);

    EXPECT_EQ(name, "node");
}

TEST(MockPlatformAdapterTest, GetParentPid) {
    MockPlatformAdapter adapter;

    EXPECT_CALL(adapter, get_parent_pid(200))
        .WillOnce(Return(100));

    auto parent = adapter.get_parent_pid(200);

    EXPECT_EQ(parent, 100);
}

TEST(MockPlatformAdapterTest, GetEnvironment) {
    MockPlatformAdapter adapter;

    std::map<std::string, std::string> test_env = {
        {"PATH", "/usr/bin:/bin"},
        {"MCP_SERVER", "true"}
    };

    EXPECT_CALL(adapter, get_environment(100))
        .WillOnce(Return(test_env));

    auto env = adapter.get_environment(100);

    EXPECT_EQ(env.size(), 2);
    EXPECT_EQ(env["MCP_SERVER"], "true");
}

TEST(MockPlatformAdapterTest, HasBidirectionalPipes) {
    MockPlatformAdapter adapter;

    EXPECT_CALL(adapter, has_bidirectional_pipes(100))
        .WillOnce(Return(true));

    EXPECT_CALL(adapter, has_bidirectional_pipes(200))
        .WillOnce(Return(false));

    EXPECT_TRUE(adapter.has_bidirectional_pipes(100));
    EXPECT_FALSE(adapter.has_bidirectional_pipes(200));
}

TEST(MockPlatformAdapterTest, GetListeningSockets) {
    MockPlatformAdapter adapter;

    NetworkListener listener1;
    listener1.pid = 100;
    listener1.address = "127.0.0.1";
    listener1.port = 3000;
    listener1.protocol = "tcp";
    listener1.process_name = "node";

    std::vector<NetworkListener> listeners = {listener1};
    adapter.set_listening_sockets(listeners);

    auto result = adapter.get_listening_sockets();

    ASSERT_THAT(result, SizeIs(1));
    EXPECT_EQ(result[0].pid, 100);
    EXPECT_EQ(result[0].port, 3000);
}

// ============================================================================
// Platform-specific Tests
// ============================================================================

TEST(PlatformAdapterTest, PlatformName) {
    MockPlatformAdapter adapter;

    EXPECT_CALL(adapter, platform_name())
        .WillOnce(Return("test-platform"));

    auto platform = adapter.platform_name();
    EXPECT_EQ(platform, "test-platform");
}

TEST(PlatformAdapterTest, FileExists) {
    MockPlatformAdapter adapter;

    EXPECT_CALL(adapter, file_exists("/path/to/file"))
        .WillOnce(Return(true));

    EXPECT_CALL(adapter, file_exists("/path/to/nonexistent"))
        .WillOnce(Return(false));

    EXPECT_TRUE(adapter.file_exists("/path/to/file"));
    EXPECT_FALSE(adapter.file_exists("/path/to/nonexistent"));
}

TEST(PlatformAdapterTest, ExpandPath) {
    MockPlatformAdapter adapter;

    EXPECT_CALL(adapter, expand_path("~/file.txt"))
        .WillOnce(Return("/home/user/file.txt"));

    auto expanded = adapter.expand_path("~/file.txt");
    EXPECT_EQ(expanded, "/home/user/file.txt");
}
