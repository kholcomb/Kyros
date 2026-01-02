#ifndef KYROS_RULEPACK_HPP
#define KYROS_RULEPACK_HPP

#include <kyros/candidate.hpp>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace kyros {

/**
 * Match condition for a rule
 */
struct RuleMatch {
    // Match type
    enum class Type {
        ProcessName,        // Match against process_name
        CommandContains,    // Command line contains string
        CommandRegex,       // Command line matches regex
        PortEquals,         // Port number equals
        URLContains,        // URL contains string
        ConfigFile,         // Declared in specific config file
        EvidenceType,       // Has specific evidence type
        ParentProcess       // Parent process name matches
    };

    Type type;
    std::string value;

    // Check if candidate matches this condition
    bool matches(const Candidate& candidate) const;
};

/**
 * Action to take when rule matches
 */
struct RuleAction {
    // Action type
    enum class Type {
        AddEvidence,           // Add new evidence
        BoostConfidence,       // Multiply confidence by factor
        SetMinimumConfidence,  // Ensure minimum confidence level
        AddTag,                // Add descriptive tag

        // NEW: Exclusion actions for false positive filtering
        AddNegativeEvidence,   // Mark as NOT MCP (confirmed false positive)
        SetMaximumConfidence,  // Cap confidence (soft exclusion)
        Exclude                // Hard exclude (confidence = 0)
    };

    Type type;

    // For AddEvidence
    std::string evidence_type;
    std::string evidence_description;
    double evidence_confidence = 0.0;
    std::string evidence_source;

    // For BoostConfidence
    double boost_factor = 1.0;

    // For SetMinimumConfidence
    double minimum_confidence = 0.0;

    // For AddTag
    std::string tag;

    // For AddNegativeEvidence
    std::string negative_evidence_type;
    std::string negative_evidence_description;
    double negative_evidence_confidence = 0.99;

    // For SetMaximumConfidence
    double maximum_confidence = 0.0;

    // Apply this action to a candidate
    void apply(Candidate& candidate) const;
};

/**
 * A single detection rule
 */
struct Rule {
    std::string name;
    std::string description;
    std::vector<RuleMatch> match_conditions;  // All conditions must match (AND)
    std::vector<RuleAction> actions;

    // Check if all match conditions are satisfied
    bool matches(const Candidate& candidate) const;

    // Apply all actions to the candidate
    void apply(Candidate& candidate) const;
};

/**
 * A collection of rules
 */
struct Rulepack {
    std::string name;
    std::string version;
    std::string description;
    std::vector<Rule> rules;

    // Load from JSON file
    static Rulepack load_from_file(const std::string& path);

    // Load from JSON string
    static Rulepack load_from_json(const nlohmann::json& json);

    // Apply all rules to a candidate
    void apply(Candidate& candidate) const;
};

/**
 * Manages multiple rulepacks
 */
class RuleEngine {
public:
    // Add a rulepack
    void add_rulepack(const Rulepack& rulepack);

    // Load rulepack from file
    void load_rulepack(const std::string& path);

    // Apply all rulepacks to a candidate
    void apply(Candidate& candidate) const;

    // Get all loaded rulepacks
    const std::vector<Rulepack>& rulepacks() const { return rulepacks_; }

private:
    std::vector<Rulepack> rulepacks_;
};

} // namespace kyros

#endif // KYROS_RULEPACK_HPP
