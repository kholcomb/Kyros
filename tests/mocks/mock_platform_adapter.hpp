/**
 * Mock Platform Adapter for Testing
 * Provides controlled platform information for unit tests
 */

#pragma once

#include <gmock/gmock.h>
#include <kyros/platform/platform_adapter.hpp>
#include <kyros/platform/process.hpp>
#include <vector>
#include <string>

namespace kyros::test {

class MockPlatformAdapter : public kyros::PlatformAdapter {
public:
    // Platform identification
    MOCK_METHOD(std::string, platform_name, (), (const, override));

    // File system operations
    MOCK_METHOD(bool, file_exists, (const std::string&), (override));
    MOCK_METHOD(std::string, expand_path, (const std::string&), (override));
    MOCK_METHOD(nlohmann::json, read_json_file, (const std::string&), (override));
    MOCK_METHOD(std::vector<std::string>, list_directory, (const std::string&), (override));

    // Process operations
    MOCK_METHOD(std::vector<int>, get_process_list, (), (override));
    MOCK_METHOD(std::string, get_command_line, (int), (override));
    MOCK_METHOD(std::string, get_process_name, (int), (override));
    MOCK_METHOD(int, get_parent_pid, (int), (override));
    MOCK_METHOD((std::map<std::string, std::string>), get_environment, (int), (override));
    MOCK_METHOD(bool, has_bidirectional_pipes, (int), (override));

    // Network operations
    MOCK_METHOD(std::vector<NetworkListener>, get_listening_sockets, (), (override));

    // Process spawning
    MOCK_METHOD((std::unique_ptr<Process>), spawn_process_with_pipes,
                (const std::string&, const std::vector<std::string>&), (override));

    // Container support
    MOCK_METHOD(std::vector<DockerContainer>, docker_list_containers, (), (override));
    MOCK_METHOD(std::vector<KubernetesPod>, k8s_list_pods, (), (override));

    // Helper methods to set up test data
    void set_process_list(const std::vector<int>& pids) {
        test_pids_ = pids;
        ON_CALL(*this, get_process_list())
            .WillByDefault(testing::Return(test_pids_));
    }

    void set_listening_sockets(const std::vector<NetworkListener>& listeners) {
        test_listeners_ = listeners;
        ON_CALL(*this, get_listening_sockets())
            .WillByDefault(testing::Return(test_listeners_));
    }

private:
    std::vector<int> test_pids_;
    std::vector<NetworkListener> test_listeners_;
};

} // namespace kyros::test
