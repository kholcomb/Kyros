// macOS Platform Adapter Implementation

#include <kyros/platform/platform_adapter.hpp>
#include <kyros/platform/macos/macos_process.hpp>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <pwd.h>
#include <wordexp.h>
#include <sys/wait.h>
#include <signal.h>
#include <vector>
#include <libproc.h>
#include <sys/proc_info.h>
#include <libgen.h>
#include <array>
#include <cstring>

namespace kyros {

class MacOSPlatformAdapter : public PlatformAdapter {
public:
    std::string platform_name() const override {
        return "macOS";
    }

    bool file_exists(const std::string& path) override {
        std::filesystem::path fs_path(path);
        return std::filesystem::exists(fs_path);
    }

    std::string expand_path(const std::string& path) override {
        if (path.empty()) {
            return path;
        }

        // Manual ~ expansion (more reliable than wordexp for paths with spaces)
        if (path[0] == '~') {
            const char* home = std::getenv("HOME");
            if (!home) {
                struct passwd* pw = getpwuid(getuid());
                home = pw ? pw->pw_dir : nullptr;
            }
            if (home) {
                if (path.length() == 1) {
                    return std::string(home);
                } else if (path[1] == '/') {
                    return std::string(home) + path.substr(1);
                }
            }
        }

        // Manual environment variable expansion (basic support for $VAR and ${VAR})
        std::string result = path;
        size_t pos = 0;
        while ((pos = result.find('$', pos)) != std::string::npos) {
            size_t end_pos = pos + 1;
            bool has_braces = (end_pos < result.length() && result[end_pos] == '{');

            if (has_braces) {
                end_pos++;
                size_t close_brace = result.find('}', end_pos);
                if (close_brace != std::string::npos) {
                    std::string var_name = result.substr(end_pos, close_brace - end_pos);
                    const char* var_value = std::getenv(var_name.c_str());
                    if (var_value) {
                        result = result.substr(0, pos) + std::string(var_value) + result.substr(close_brace + 1);
                    } else {
                        pos = close_brace + 1;
                    }
                } else {
                    break;
                }
            } else {
                // Find end of variable name (alphanumeric and underscore)
                while (end_pos < result.length() &&
                       (isalnum(result[end_pos]) || result[end_pos] == '_')) {
                    end_pos++;
                }
                if (end_pos > pos + 1) {
                    std::string var_name = result.substr(pos + 1, end_pos - pos - 1);
                    const char* var_value = std::getenv(var_name.c_str());
                    if (var_value) {
                        result = result.substr(0, pos) + std::string(var_value) + result.substr(end_pos);
                    } else {
                        pos = end_pos;
                    }
                } else {
                    pos++;
                }
            }
        }

        return result;
    }

    nlohmann::json read_json_file(const std::string& path) override {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }

        nlohmann::json result;
        try {
            file >> result;
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error("Failed to parse JSON from " + path + ": " + e.what());
        }

        return result;
    }

    std::vector<std::string> list_directory(const std::string& path) override {
        std::vector<std::string> result;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                result.push_back(entry.path().filename().string());
            }
        } catch (const std::filesystem::filesystem_error& e) {
            throw std::runtime_error(std::string("Failed to list directory: ") + e.what());
        }
        return result;
    }

    std::vector<int> get_process_list() override {
        std::vector<int> result;

        // Get number of processes
        int num_pids = proc_listallpids(nullptr, 0);
        if (num_pids <= 0) {
            return result;  // Return empty vector if failed
        }

        // Allocate buffer for PIDs
        std::vector<pid_t> pids(num_pids);

        // Get actual PIDs
        num_pids = proc_listallpids(pids.data(), static_cast<int>(pids.size() * sizeof(pid_t)));
        if (num_pids <= 0) {
            return result;
        }

        // Convert to int vector
        for (int i = 0; i < num_pids; i++) {
            if (pids[i] > 0) {
                result.push_back(static_cast<int>(pids[i]));
            }
        }

        return result;
    }

    std::string get_command_line(int pid) override {
        // Use ps command to get full command line
        std::string cmd = "ps -p " + std::to_string(pid) + " -o command=";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return "";
        }

        std::string result;
        std::array<char, 4096> buffer;
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }

        pclose(pipe);

        // Trim trailing newline
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }

        return result;
    }

    std::string get_process_name(int pid) override {
        char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
        int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
        if (ret <= 0) {
            return "";
        }

        // Extract just the filename from the path
        char* name = basename(pathbuf);
        return std::string(name ? name : "");
    }

    int get_parent_pid(int pid) override {
        struct proc_bsdinfo proc_info;
        int ret = proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &proc_info, sizeof(proc_info));
        if (ret <= 0) {
            return -1;
        }
        return static_cast<int>(proc_info.pbi_ppid);
    }

    std::map<std::string, std::string> get_environment(int pid) override {
        // Note: Getting environment variables of another process is difficult on macOS
        // and often requires elevated privileges. For now, return empty map.
        // This is acceptable since environment is just one piece of evidence
        // and ProcessDetectionEngine uses multiple signals.
        (void)pid;  // Suppress unused parameter warning
        return std::map<std::string, std::string>();
    }

    bool has_bidirectional_pipes(int pid) override {
        // Get buffer size needed
        int buffer_size = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, nullptr, 0);
        if (buffer_size <= 0) {
            return false;
        }

        // Allocate buffer for file descriptors
        std::vector<char> buffer(buffer_size);
        int ret = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, buffer.data(), buffer_size);
        if (ret <= 0) {
            return false;
        }

        // Parse file descriptors
        auto* fd_info = reinterpret_cast<proc_fdinfo*>(buffer.data());
        int num_fds = ret / static_cast<int>(PROC_PIDLISTFD_SIZE);

        bool stdin_is_pipe = false;
        bool stdout_is_pipe = false;

        for (int i = 0; i < num_fds; i++) {
            if (fd_info[i].proc_fd == 0 && fd_info[i].proc_fdtype == PROX_FDTYPE_PIPE) {
                stdin_is_pipe = true;
            }
            if (fd_info[i].proc_fd == 1 && fd_info[i].proc_fdtype == PROX_FDTYPE_PIPE) {
                stdout_is_pipe = true;
            }
        }

        return stdin_is_pipe && stdout_is_pipe;
    }

    std::vector<NetworkListener> get_listening_sockets() override {
        std::vector<NetworkListener> result;

        // Use lsof to find listening sockets
        FILE* pipe = popen("lsof -i -n -P | grep LISTEN", "r");
        if (!pipe) {
            return result;
        }

        std::array<char, 4096> buffer;
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            // Parse lsof output line
            // Format: COMMAND  PID USER  FD  TYPE DEVICE SIZE/OFF NODE NAME
            // Example: node    1234 user   12u  IPv4 0x...      0t0  TCP *:3000 (LISTEN)

            std::string line(buffer.data());
            std::istringstream iss(line);

            std::string command, pid_str, user, fd, type, device, size, node, name;
            iss >> command >> pid_str >> user >> fd >> type >> device >> size >> node >> name;

            if (name.empty()) continue;

            // Parse address and port from name (format: *:PORT or IP:PORT)
            size_t colon_pos = name.find_last_of(':');
            if (colon_pos == std::string::npos) continue;

            std::string address = name.substr(0, colon_pos);
            std::string port_str = name.substr(colon_pos + 1);

            // Remove (LISTEN) suffix if present
            size_t listen_pos = port_str.find(" (LISTEN)");
            if (listen_pos != std::string::npos) {
                port_str = port_str.substr(0, listen_pos);
            }

            try {
                NetworkListener listener;
                listener.pid = std::stoi(pid_str);
                listener.address = (address == "*") ? "0.0.0.0" : address;
                listener.port = std::stoi(port_str);
                // Fix: Check 'node' field (which contains TCP/UDP), not 'type' (which contains IPv4/IPv6)
                listener.protocol = (node.find("TCP") != std::string::npos) ? "tcp" : "udp";

                result.push_back(listener);
            } catch (...) {
                // Skip malformed entries
                continue;
            }
        }

        pclose(pipe);
        return result;
    }

    std::unique_ptr<Process> spawn_process_with_pipes(
        const std::string& command,
        const std::vector<std::string>& args) override {

        // Create pipes: [0] = read end, [1] = write end
        int stdin_pipe[2];
        int stdout_pipe[2];
        int stderr_pipe[2];

        if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0) {
            throw std::runtime_error("Failed to create pipes");
        }

        pid_t pid = fork();

        if (pid < 0) {
            // Fork failed
            close(stdin_pipe[0]); close(stdin_pipe[1]);
            close(stdout_pipe[0]); close(stdout_pipe[1]);
            close(stderr_pipe[0]); close(stderr_pipe[1]);
            throw std::runtime_error("Failed to fork process");
        }

        if (pid == 0) {
            // Child process

            // Redirect stdin: close write end, dup read end to stdin
            close(stdin_pipe[1]);
            dup2(stdin_pipe[0], STDIN_FILENO);
            close(stdin_pipe[0]);

            // Redirect stdout: close read end, dup write end to stdout
            close(stdout_pipe[0]);
            dup2(stdout_pipe[1], STDOUT_FILENO);
            close(stdout_pipe[1]);

            // Redirect stderr: close read end, dup write end to stderr
            close(stderr_pipe[0]);
            dup2(stderr_pipe[1], STDERR_FILENO);
            close(stderr_pipe[1]);

            // Build argument vector for execvp
            std::vector<char*> argv;
            argv.push_back(const_cast<char*>(command.c_str()));
            for (const auto& arg : args) {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
            argv.push_back(nullptr);

            // Execute command
            execvp(command.c_str(), argv.data());

            // If execvp returns, it failed
            std::cerr << "Failed to execute: " << command << std::endl;
            _exit(1);
        }

        // Parent process

        // Close unused pipe ends
        close(stdin_pipe[0]);  // Don't need read end of stdin
        close(stdout_pipe[1]); // Don't need write end of stdout
        close(stderr_pipe[1]); // Don't need write end of stderr

        // Create and return Process object
        return std::make_unique<MacOSProcess>(
            pid,
            stdin_pipe[1],  // Write to child's stdin
            stdout_pipe[0], // Read from child's stdout
            stderr_pipe[0]  // Read from child's stderr
        );
    }

    std::vector<DockerContainer> docker_list_containers() override {
        std::vector<DockerContainer> result;

        // Check Docker availability
        if (std::system("docker info > /dev/null 2>&1") != 0) {
            return result;
        }

        // List containers: docker ps --format json
        FILE* pipe = popen("docker ps --format json", "r");
        if (!pipe) {
            return result;
        }

        // Parse each JSON line and call docker inspect
        std::array<char, 4096> buffer;
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            try {
                auto json_obj = nlohmann::json::parse(buffer.data());
                std::string container_id = json_obj["ID"];

                // Get detailed metadata
                DockerContainer container = docker_inspect_container(container_id);
                if (!container.id.empty()) {
                    result.push_back(container);
                }
            } catch (...) {
                continue; // Skip malformed entries
            }
        }

        pclose(pipe);
        return result;
    }

    std::vector<std::string> get_docker_mcp_servers() override {
        std::vector<std::string> result;

        // Check if docker mcp is available
        if (std::system("docker mcp version > /dev/null 2>&1") != 0) {
            return result;  // docker mcp not available
        }

        // Get MCP server list: docker mcp server list
        // Note: The exact output format needs to be determined from actual docker mcp CLI
        FILE* pipe = popen("docker mcp server list --format json 2>/dev/null", "r");
        if (!pipe) {
            return result;
        }

        // Parse output - extract container IDs or names
        std::array<char, 4096> buffer;
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            try {
                auto json = nlohmann::json::parse(buffer.data());
                // Try different field names that might contain the container ID
                if (json.contains("container_id")) {
                    result.push_back(json["container_id"]);
                } else if (json.contains("id")) {
                    result.push_back(json["id"]);
                } else if (json.contains("name")) {
                    result.push_back(json["name"]);
                }
            } catch (...) {
                // Skip malformed entries
                continue;
            }
        }

        pclose(pipe);
        return result;
    }

private:
    DockerContainer docker_inspect_container(const std::string& id) {
        DockerContainer container;

        std::string cmd = "docker inspect " + id;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return container;
        }

        // Read full JSON output
        std::string json_str;
        std::array<char, 4096> buffer;
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            json_str += buffer.data();
        }
        pclose(pipe);

        try {
            auto json_arr = nlohmann::json::parse(json_str);
            if (json_arr.empty() || !json_arr.is_array()) {
                return container;
            }

            auto inspect = json_arr[0];

            // Populate basic fields
            container.id = inspect.value("Id", "");
            container.name = inspect.value("Name", "");

            // Image
            if (inspect.contains("Config") && inspect["Config"].contains("Image")) {
                container.image = inspect["Config"]["Image"];
            }

            // Entrypoint and command
            if (inspect.contains("Config")) {
                auto config = inspect["Config"];

                // Entrypoint path
                if (config.contains("Path")) {
                    container.entrypoint_path = config["Path"];
                }

                // Entrypoint args
                if (config.contains("Args") && config["Args"].is_array()) {
                    for (const auto& arg : config["Args"]) {
                        container.entrypoint_args.push_back(arg);
                    }
                }

                // Build combined command string (legacy)
                container.command = container.entrypoint_path;
                for (const auto& arg : container.entrypoint_args) {
                    container.command += " " + arg;
                }

                // Labels
                if (config.contains("Labels") && config["Labels"].is_object()) {
                    for (auto& [key, val] : config["Labels"].items()) {
                        if (val.is_string()) {
                            container.labels[key] = val;
                        }
                    }
                }

                // Environment variables
                if (config.contains("Env") && config["Env"].is_array()) {
                    for (const auto& env_str : config["Env"]) {
                        if (env_str.is_string()) {
                            std::string env = env_str;
                            size_t eq_pos = env.find('=');
                            if (eq_pos != std::string::npos) {
                                std::string key = env.substr(0, eq_pos);
                                std::string value = env.substr(eq_pos + 1);
                                container.env[key] = value;
                            }
                        }
                    }
                }
            }

        } catch (const nlohmann::json::exception& e) {
            // Return partially populated container
            (void)e;  // Suppress unused variable warning
        }

        return container;
    }
};

std::unique_ptr<PlatformAdapter> create_platform_adapter() {
    return std::make_unique<MacOSPlatformAdapter>();
}

} // namespace kyros
