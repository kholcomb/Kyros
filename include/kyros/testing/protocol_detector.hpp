#ifndef KYROS_PROTOCOL_DETECTOR_HPP
#define KYROS_PROTOCOL_DETECTOR_HPP

#include <kyros/candidate.hpp>
#include <kyros/platform/process.hpp>
#include <chrono>
#include <string>
#include <vector>

namespace kyros {

/**
 * Protocol types that can be detected
 */
enum class ProtocolType {
    Unknown,        // Cannot determine protocol type
    MCP,            // Model Context Protocol (valid MCP server)
    ChromiumIPC,    // Chromium Inter-Process Communication (Electron/VSCode helpers)
    LSP,            // Language Server Protocol (LSP servers)
    GenericJSONRPC, // Generic JSON-RPC (not MCP-specific)
    Binary,         // Binary protocol (not text-based)
    Invalid         // Invalid or unparseable protocol
};

/**
 * Result of protocol detection
 */
struct ProtocolSignature {
    ProtocolType type;
    std::string name;
    double confidence;           // How confident we are in this detection (0.0 - 1.0)
    std::string detected_evidence;

    ProtocolSignature()
        : type(ProtocolType::Unknown)
        , confidence(0.0) {}

    ProtocolSignature(ProtocolType type, const std::string& name,
                     double confidence, const std::string& evidence)
        : type(type)
        , name(name)
        , confidence(confidence)
        , detected_evidence(evidence) {}
};

/**
 * Protocol detector for identifying IPC protocols
 *
 * Provides both passive detection (from process metadata only) and
 * active detection (by probing the process via stdio).
 */
class ProtocolDetector {
public:
    ProtocolDetector() = default;
    ~ProtocolDetector() = default;

    /**
     * Passive detection from process information only (no IPC communication)
     *
     * Analyzes process name and command line to identify known protocol patterns
     * without spawning or communicating with the process.
     *
     * @param candidate The candidate to analyze
     * @return Protocol signature with detection confidence
     */
    ProtocolSignature detect_from_process_info(const Candidate& candidate) const;

    /**
     * Active detection via stdio probing
     *
     * Attempts to identify the protocol by sending probes and analyzing responses.
     * This is more accurate but requires spawning the process.
     *
     * @param process Running process to probe
     * @param timeout Timeout for protocol detection
     * @return Protocol signature with detection confidence
     */
    ProtocolSignature detect_from_stdio(Process* process,
                                       std::chrono::milliseconds timeout) const;

private:
    // Passive detection helpers
    bool is_chromium_ipc_process(const Candidate& candidate) const;
    bool is_lsp_process(const Candidate& candidate) const;

    // Active detection helpers
    ProtocolSignature detect_chromium_ipc(Process* process,
                                         std::chrono::milliseconds timeout) const;
    ProtocolSignature detect_lsp(Process* process,
                                 std::chrono::milliseconds timeout) const;
    ProtocolSignature detect_mcp(Process* process,
                                 std::chrono::milliseconds timeout) const;
};

} // namespace kyros

#endif // KYROS_PROTOCOL_DETECTOR_HPP
