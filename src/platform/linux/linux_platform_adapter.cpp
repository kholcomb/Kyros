// Linux Platform Adapter Implementation
//
// TODO: Implement Linux platform support using /proc filesystem

#include <kyros/platform/platform_adapter.hpp>
#include <stdexcept>

namespace kyros {

class LinuxPlatformAdapter : public PlatformAdapter {
public:
    std::string platform_name() const override {
        return "Linux";
    }

    bool file_exists(const std::string& path) override {
        (void)path;
        throw std::runtime_error("Linux platform not yet implemented");
    }

    std::string expand_path(const std::string& path) override {
        (void)path;
        throw std::runtime_error("Linux platform not yet implemented");
    }

    nlohmann::json read_json_file(const std::string& path) override {
        (void)path;
        throw std::runtime_error("Linux platform not yet implemented");
    }

    std::vector<std::string> list_directory(const std::string& path) override {
        (void)path;
        throw std::runtime_error("Linux platform not yet implemented");
    }

    std::vector<int> get_process_list() override {
        throw std::runtime_error("Linux platform not yet implemented");
    }

    std::string get_command_line(int pid) override {
        (void)pid;
        throw std::runtime_error("Linux platform not yet implemented");
    }

    std::string get_process_name(int pid) override {
        (void)pid;
        throw std::runtime_error("Linux platform not yet implemented");
    }

    int get_parent_pid(int pid) override {
        (void)pid;
        throw std::runtime_error("Linux platform not yet implemented");
    }

    std::map<std::string, std::string> get_environment(int pid) override {
        (void)pid;
        throw std::runtime_error("Linux platform not yet implemented");
    }

    bool has_bidirectional_pipes(int pid) override {
        (void)pid;
        throw std::runtime_error("Linux platform not yet implemented");
    }

    std::vector<NetworkListener> get_listening_sockets() override {
        throw std::runtime_error("Linux platform not yet implemented");
    }

    std::unique_ptr<Process> spawn_process_with_pipes(
        const std::string& command,
        const std::vector<std::string>& args) override {
        (void)command;
        (void)args;
        throw std::runtime_error("Linux platform not yet implemented");
    }

    std::vector<DockerContainer> docker_list_containers() override {
        throw std::runtime_error("Linux platform not yet implemented");
    }

    std::vector<std::string> get_docker_mcp_servers() override {
        throw std::runtime_error("Linux platform not yet implemented");
    }
};

std::unique_ptr<PlatformAdapter> create_platform_adapter() {
    return std::make_unique<LinuxPlatformAdapter>();
}

} // namespace kyros
