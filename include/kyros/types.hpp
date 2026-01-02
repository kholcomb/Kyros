#ifndef KYROS_TYPES_HPP
#define KYROS_TYPES_HPP

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <map>

#include <nlohmann/json.hpp>

namespace kyros {

// Forward declarations
class Scanner;
class DetectionEngine;
class TestingEngine;
class Reporter;
class PlatformAdapter;
class Process;
class ServerInterrogator;

// Scan modes
enum class ScanMode {
    PassiveOnly,       // Default - discovery only
    ActiveOnly,        // Test pre-provided candidates
    PassiveThenActive  // Discovery + confirmation (--active flag)
};

// Transport types
enum class TransportType {
    Stdio,
    Http,
    Sse,
    Unknown
};

// Container types
struct DockerContainer {
    std::string id;
    std::string name;
    std::string image;
    std::string command;  // Combined command string (legacy)

    // From docker inspect:
    std::string entrypoint_path;               // Config.Path (entrypoint executable)
    std::vector<std::string> entrypoint_args;  // Config.Args (command arguments)

    std::map<std::string, std::string> labels;  // Container labels
    std::map<std::string, std::string> env;     // Environment variables
};

struct KubernetesPod {
    std::string name;
    std::string namespace_name;
    std::string pod_ip;
    std::vector<std::string> container_names;
    std::map<std::string, std::string> annotations;
    std::map<std::string, std::string> labels;
};

// Network listener
struct NetworkListener {
    int pid;
    std::string address;
    int port;
    std::string protocol;  // "tcp" or "udp"
    std::string process_name;
};

// Timestamp type
using Timestamp = std::chrono::system_clock::time_point;

// Duration type
using Duration = std::chrono::duration<double>;

} // namespace kyros

#endif // KYROS_TYPES_HPP
