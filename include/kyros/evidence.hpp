#ifndef KYROS_EVIDENCE_HPP
#define KYROS_EVIDENCE_HPP

#include <string>

namespace kyros {

/**
 * Evidence supporting an MCP server detection
 */
struct Evidence {
    /**
     * Strength classification for evidence
     *
     * Determines how evidence contributes to confidence calculations:
     * - Definitive: 100% certain (e.g., config_declared, active_mcp_response)
     * - Strong: High confidence standalone (e.g., official_mcp_package)
     * - Moderate: Needs corroboration (e.g., file_descriptors, environment)
     * - Weak: Must combine with others (e.g., parent_process alone)
     */
    enum class Strength {
        Definitive,   // 100% certain indicators
        Strong,       // High confidence, can stand alone
        Moderate,     // Needs corroboration from other evidence
        Weak          // Must combine with other evidence types
    };

    std::string type;         // Evidence type (e.g., "config_file", "process_pipe")
    std::string description;  // Human-readable description
    double confidence;        // Confidence score (0.0 - 1.0)
    std::string source;       // Source of evidence (file path, PID, etc.)
    Strength strength;        // Strength classification
    bool is_negative;         // True for negative evidence (confirmed NOT MCP)

    Evidence() = default;

    Evidence(const std::string& type,
             const std::string& description,
             double confidence,
             const std::string& source = "",
             Strength strength = Strength::Moderate,
             bool is_negative = false)
        : type(type)
        , description(description)
        , confidence(confidence)
        , source(source)
        , strength(strength)
        , is_negative(is_negative) {}
};

} // namespace kyros

#endif // KYROS_EVIDENCE_HPP
