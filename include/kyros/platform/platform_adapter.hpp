#ifndef KYROS_PLATFORM_ADAPTER_HPP
#define KYROS_PLATFORM_ADAPTER_HPP

#include <kyros/types.hpp>
#include <kyros/platform/process.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace kyros {

/**
 * Platform abstraction layer
 *
 * Provides cross-platform access to OS-level functionality
 */
class PlatformAdapter {
public:
    virtual ~PlatformAdapter() = default;

    // Platform identification
    virtual std::string platform_name() const = 0;

    // File system operations
    virtual bool file_exists(const std::string& path) = 0;
    virtual std::string expand_path(const std::string& path) = 0;
    virtual nlohmann::json read_json_file(const std::string& path) = 0;
    virtual std::vector<std::string> list_directory(const std::string& path) = 0;

    // Process operations
    virtual std::vector<int> get_process_list() = 0;
    virtual std::string get_command_line(int pid) = 0;
    virtual std::string get_process_name(int pid) = 0;
    virtual int get_parent_pid(int pid) = 0;
    virtual std::map<std::string, std::string> get_environment(int pid) = 0;
    virtual bool has_bidirectional_pipes(int pid) = 0;

    // Network operations
    virtual std::vector<NetworkListener> get_listening_sockets() = 0;

    // Process spawning
    virtual std::unique_ptr<Process> spawn_process_with_pipes(
        const std::string& command,
        const std::vector<std::string>& args = {}) = 0;

    // Container support (optional)
    virtual std::vector<DockerContainer> docker_list_containers() {
        return {};
    }

    virtual std::vector<std::string> get_docker_mcp_servers() {
        return {};  // Default: docker mcp not available
    }

    virtual std::vector<KubernetesPod> k8s_list_pods() {
        return {};
    }

protected:
    PlatformAdapter() = default;
};

/**
 * Factory function to create platform-appropriate adapter
 */
std::unique_ptr<PlatformAdapter> create_platform_adapter();

} // namespace kyros

#endif // KYROS_PLATFORM_ADAPTER_HPP
