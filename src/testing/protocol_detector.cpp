#include <kyros/testing/protocol_detector.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <iostream>
#include <unistd.h>  // For getpid()

namespace kyros {

// ============================================================================
// Passive Detection (from process info only - no IPC)
// ============================================================================

bool ProtocolDetector::is_chromium_ipc_process(const Candidate& candidate) const {
    // Chromium helper processes have distinctive patterns in process name and command line
    static const std::vector<std::string> chromium_patterns = {
        "Helper (GPU)",
        "Helper (Renderer)",
        "Helper (Plugin)",
        "Helper (Network Service)",
        "Helper (Utility)",
        "--type=gpu-process",
        "--type=renderer",
        "--type=utility",
        "--type=zygote",
        "--enable-crashpad",
        "--enable-crash-reporter"
    };

    for (const auto& pattern : chromium_patterns) {
        if (candidate.process_name.find(pattern) != std::string::npos ||
            candidate.command.find(pattern) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool ProtocolDetector::is_lsp_process(const Candidate& candidate) const {
    // Language Server Protocol servers have distinctive patterns
    static const std::vector<std::string> lsp_patterns = {
        "vscode-html-language-server",
        "vscode-json-language-server",
        "vscode-css-language-server",
        "typescript-language-server",
        "eslint-language-server",
        "language-server",
        "languageserver",
        "--stdio"  // Common LSP flag
    };

    // Check for LSP-specific patterns
    for (const auto& pattern : lsp_patterns) {
        if (candidate.process_name.find(pattern) != std::string::npos ||
            candidate.command.find(pattern) != std::string::npos) {
            return true;
        }
    }

    // Check for --node-ipc flag combined with extension paths (LSP servers)
    if (candidate.command.find("--node-ipc") != std::string::npos &&
        (candidate.command.find(".vscode/extensions") != std::string::npos ||
         candidate.command.find("language-features") != std::string::npos)) {
        return true;
    }

    return false;
}

ProtocolSignature ProtocolDetector::detect_from_process_info(const Candidate& candidate) const {
    // Check for Chromium IPC patterns
    if (is_chromium_ipc_process(candidate)) {
        return ProtocolSignature(
            ProtocolType::ChromiumIPC,
            "Chromium IPC",
            0.95,
            "Chromium helper process pattern detected in process name/command"
        );
    }

    // Check for LSP patterns
    if (is_lsp_process(candidate)) {
        return ProtocolSignature(
            ProtocolType::LSP,
            "Language Server Protocol",
            0.90,
            "LSP server pattern detected in process name/command"
        );
    }

    // Unknown - cannot determine from passive analysis
    return ProtocolSignature(
        ProtocolType::Unknown,
        "Unknown",
        0.0,
        "No distinctive protocol patterns found"
    );
}

// ============================================================================
// Active Detection (via stdio probing)
// ============================================================================

ProtocolSignature ProtocolDetector::detect_chromium_ipc(Process* process,
                                                        std::chrono::milliseconds timeout) const {
    // Chromium IPC uses a binary protocol
    // Try to read initial bytes - if we get binary data or nothing, likely Chromium IPC

    try {
        std::string initial_data = process->read_stdout_line(timeout);

        // Chromium IPC doesn't respond to text probes - empty or binary response
        if (initial_data.empty()) {
            return ProtocolSignature(
                ProtocolType::ChromiumIPC,
                "Chromium IPC",
                0.80,
                "No text response on stdio (binary protocol)"
            );
        }

        // Check for null bytes or non-printable characters (binary protocol)
        bool has_binary = std::any_of(initial_data.begin(), initial_data.end(),
                                     [](char c) { return c == '\0' || (c < 32 && c != '\n' && c != '\r' && c != '\t'); });

        if (has_binary) {
            return ProtocolSignature(
                ProtocolType::Binary,
                "Binary Protocol",
                0.85,
                "Binary data detected on stdio"
            );
        }

    } catch (const std::exception&) {
        // Timeout or error - might be Chromium IPC that doesn't respond
        return ProtocolSignature(
            ProtocolType::ChromiumIPC,
            "Chromium IPC",
            0.60,
            "No response on stdio probe"
        );
    }

    return ProtocolSignature(ProtocolType::Unknown, "Unknown", 0.0, "");
}

ProtocolSignature ProtocolDetector::detect_lsp(Process* process,
                                              std::chrono::milliseconds timeout) const {
    // LSP uses JSON-RPC with Content-Length headers
    // Try sending an LSP initialize request

    try {
        nlohmann::json lsp_init = {
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", "initialize"},
            {"params", {
                {"processId", static_cast<int>(getpid())},
                {"rootUri", nullptr},
                {"capabilities", nlohmann::json::object()}
            }}
        };

        std::string request_body = lsp_init.dump();
        std::string request = "Content-Length: " + std::to_string(request_body.size())
                            + "\r\n\r\n" + request_body;

        process->write_stdin(request);

        // LSP responses start with "Content-Length: "
        std::string response = process->read_stdout_line(timeout);

        if (response.find("Content-Length:") == 0) {
            return ProtocolSignature(
                ProtocolType::LSP,
                "Language Server Protocol",
                0.95,
                "Content-Length header detected in response"
            );
        }

    } catch (const std::exception&) {
        // Not LSP
    }

    return ProtocolSignature(ProtocolType::Unknown, "Unknown", 0.0, "");
}

ProtocolSignature ProtocolDetector::detect_mcp(Process* process,
                                              std::chrono::milliseconds timeout) const {
    // MCP uses newline-delimited JSON-RPC (no Content-Length headers)

    try {
        nlohmann::json mcp_init = {
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", "initialize"},
            {"params", {
                {"protocolVersion", "2024-11-05"},
                {"capabilities", nlohmann::json::object()},
                {"clientInfo", {
                    {"name", "Kyros"},
                    {"version", "2.0.0"}
                }}
            }}
        };

        std::string request = mcp_init.dump() + "\n";
        process->write_stdin(request);

        std::string response_line = process->read_stdout_line(timeout);

        if (response_line.empty()) {
            return ProtocolSignature(ProtocolType::Unknown, "Unknown", 0.0, "No response");
        }

        // Try to parse as JSON
        nlohmann::json response;
        try {
            response = nlohmann::json::parse(response_line);
        } catch (const nlohmann::json::parse_error&) {
            return ProtocolSignature(
                ProtocolType::Invalid,
                "Invalid",
                0.0,
                "Response is not valid JSON"
            );
        }

        // Validate it's JSON-RPC 2.0
        if (response.value("jsonrpc", "") != "2.0") {
            return ProtocolSignature(
                ProtocolType::GenericJSONRPC,
                "Generic JSON-RPC",
                0.50,
                "Valid JSON but not JSON-RPC 2.0"
            );
        }

        // Check for MCP-specific initialize response structure
        if (response.contains("result")) {
            const auto& result = response["result"];

            // MCP initialize response has protocolVersion in result
            if (result.contains("protocolVersion")) {
                return ProtocolSignature(
                    ProtocolType::MCP,
                    "Model Context Protocol",
                    0.99,
                    "Valid MCP initialize response with protocolVersion"
                );
            }

            // Has serverInfo which is MCP-specific
            if (result.contains("serverInfo")) {
                return ProtocolSignature(
                    ProtocolType::MCP,
                    "Model Context Protocol",
                    0.95,
                    "Valid MCP initialize response with serverInfo"
                );
            }

            // Valid JSON-RPC response but not MCP-specific structure
            return ProtocolSignature(
                ProtocolType::GenericJSONRPC,
                "Generic JSON-RPC",
                0.60,
                "Valid JSON-RPC response but missing MCP-specific fields"
            );
        }

        // Error response - could still be MCP (server might reject our request)
        if (response.contains("error")) {
            // Even error responses from MCP servers indicate protocol awareness
            return ProtocolSignature(
                ProtocolType::MCP,
                "Model Context Protocol",
                0.75,
                "MCP error response (server exists but rejected initialize)"
            );
        }

        return ProtocolSignature(
            ProtocolType::GenericJSONRPC,
            "Generic JSON-RPC",
            0.50,
            "Valid JSON-RPC 2.0 but cannot determine if MCP"
        );

    } catch (const std::exception& e) {
        return ProtocolSignature(
            ProtocolType::Unknown,
            "Unknown",
            0.0,
            std::string("Protocol detection failed: ") + e.what()
        );
    }
}

ProtocolSignature ProtocolDetector::detect_from_stdio(Process* process,
                                                      std::chrono::milliseconds timeout) const {
    if (!process || !process->is_running()) {
        return ProtocolSignature(ProtocolType::Unknown, "Unknown", 0.0, "Process not running");
    }

    // Try MCP detection first (most specific)
    auto mcp_sig = detect_mcp(process, timeout);
    if (mcp_sig.type == ProtocolType::MCP && mcp_sig.confidence > 0.7) {
        return mcp_sig;
    }

    // Try LSP detection
    auto lsp_sig = detect_lsp(process, timeout);
    if (lsp_sig.type == ProtocolType::LSP && lsp_sig.confidence > 0.8) {
        return lsp_sig;
    }

    // Try Chromium IPC detection (binary protocol)
    auto chromium_sig = detect_chromium_ipc(process, timeout);
    if (chromium_sig.confidence > 0.6) {
        return chromium_sig;
    }

    // Return best guess or unknown
    if (mcp_sig.confidence > 0.5) return mcp_sig;
    if (lsp_sig.confidence > 0.5) return lsp_sig;
    if (chromium_sig.confidence > 0.5) return chromium_sig;

    return ProtocolSignature(ProtocolType::Unknown, "Unknown", 0.0, "Could not determine protocol");
}

} // namespace kyros
