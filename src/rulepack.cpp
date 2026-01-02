#include <kyros/rulepack.hpp>
#include <fstream>
#include <regex>
#include <stdexcept>

namespace kyros {

// RuleMatch implementation
bool RuleMatch::matches(const Candidate& candidate) const {
    switch (type) {
        case Type::ProcessName:
            return candidate.process_name.find(value) != std::string::npos;

        case Type::CommandContains:
            return candidate.command.find(value) != std::string::npos;

        case Type::CommandRegex: {
            try {
                std::regex pattern(value);
                return std::regex_search(candidate.command, pattern);
            } catch (const std::regex_error&) {
                return false;
            }
        }

        case Type::PortEquals:
            return candidate.port == std::stoi(value);

        case Type::URLContains:
            return candidate.url.find(value) != std::string::npos;

        case Type::ConfigFile:
            return candidate.config_file.find(value) != std::string::npos;

        case Type::EvidenceType:
            for (const auto& evidence : candidate.evidence) {
                if (evidence.type == value) {
                    return true;
                }
            }
            return false;

        case Type::ParentProcess:
            // Would need parent process name stored in candidate
            // For now, return false
            return false;
    }

    return false;
}

// RuleAction implementation
void RuleAction::apply(Candidate& candidate) const {
    switch (type) {
        case Type::AddEvidence: {
            Evidence evidence(
                evidence_type,
                evidence_description,
                evidence_confidence,
                evidence_source
            );
            candidate.add_evidence(evidence);
            break;
        }

        case Type::BoostConfidence: {
            // Boost existing confidence by multiplying
            double new_confidence = candidate.confidence_score * boost_factor;
            if (new_confidence > 0.99) {
                new_confidence = 0.99;
            }
            candidate.confidence_score = new_confidence;
            break;
        }

        case Type::SetMinimumConfidence: {
            if (candidate.confidence_score < minimum_confidence) {
                candidate.confidence_score = minimum_confidence;
            }
            break;
        }

        case Type::AddTag: {
            // Tags could be added to candidate metadata in the future
            // For now, add as evidence
            Evidence tag_evidence(
                "tag",
                "Tagged as: " + tag,
                0.0,  // Tags don't affect confidence
                "rulepack"
            );
            candidate.add_evidence(tag_evidence);
            break;
        }

        case Type::AddNegativeEvidence: {
            // Add negative evidence (confirmed NOT MCP)
            Evidence neg_evidence(
                negative_evidence_type,
                negative_evidence_description,
                negative_evidence_confidence,
                "rulepack:exclusion",
                Evidence::Strength::Definitive,
                true  // is_negative = true
            );
            candidate.add_evidence(neg_evidence);
            break;
        }

        case Type::SetMaximumConfidence: {
            // Cap confidence at maximum (soft exclusion)
            if (candidate.confidence_score > maximum_confidence) {
                candidate.confidence_score = maximum_confidence;
            }
            break;
        }

        case Type::Exclude: {
            // Hard exclusion - set confidence to 0
            candidate.confidence_score = 0.0;

            // Add negative evidence for transparency
            Evidence exclusion(
                "rulepack_exclusion",
                "Excluded by rulepack rule",
                0.99,
                "rulepack:exclusion",
                Evidence::Strength::Definitive,
                true  // is_negative = true
            );
            candidate.add_evidence(exclusion);
            break;
        }
    }
}

// Rule implementation
bool Rule::matches(const Candidate& candidate) const {
    // All match conditions must be satisfied (AND logic)
    for (const auto& match : match_conditions) {
        if (!match.matches(candidate)) {
            return false;
        }
    }
    return true;
}

void Rule::apply(Candidate& candidate) const {
    if (!matches(candidate)) {
        return;
    }

    // Apply all actions
    for (const auto& action : actions) {
        action.apply(candidate);
    }
}

// Rulepack implementation
Rulepack Rulepack::load_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open rulepack file: " + path);
    }

    nlohmann::json json;
    try {
        file >> json;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("Failed to parse rulepack JSON: " + std::string(e.what()));
    }

    return load_from_json(json);
}

Rulepack Rulepack::load_from_json(const nlohmann::json& json) {
    Rulepack rulepack;

    rulepack.name = json.value("name", "Unnamed Rulepack");
    rulepack.version = json.value("version", "1.0");
    rulepack.description = json.value("description", "");

    if (!json.contains("rules") || !json["rules"].is_array()) {
        throw std::runtime_error("Rulepack must contain 'rules' array");
    }

    for (const auto& rule_json : json["rules"]) {
        Rule rule;
        rule.name = rule_json.value("name", "Unnamed Rule");
        rule.description = rule_json.value("description", "");

        // Parse matches
        if (rule_json.contains("match")) {
            const auto& match_json = rule_json["match"];

            // Process name match
            if (match_json.contains("process_name")) {
                RuleMatch match;
                match.type = RuleMatch::Type::ProcessName;
                match.value = match_json["process_name"];
                rule.match_conditions.push_back(match);
            }

            // Command contains match
            if (match_json.contains("command_contains")) {
                RuleMatch match;
                match.type = RuleMatch::Type::CommandContains;
                match.value = match_json["command_contains"];
                rule.match_conditions.push_back(match);
            }

            // Command regex match
            if (match_json.contains("command_regex")) {
                RuleMatch match;
                match.type = RuleMatch::Type::CommandRegex;
                match.value = match_json["command_regex"];
                rule.match_conditions.push_back(match);
            }

            // Port match
            if (match_json.contains("port")) {
                RuleMatch match;
                match.type = RuleMatch::Type::PortEquals;
                match.value = std::to_string(match_json["port"].get<int>());
                rule.match_conditions.push_back(match);
            }

            // URL contains match
            if (match_json.contains("url_contains")) {
                RuleMatch match;
                match.type = RuleMatch::Type::URLContains;
                match.value = match_json["url_contains"];
                rule.match_conditions.push_back(match);
            }

            // Config file match
            if (match_json.contains("config_file")) {
                RuleMatch match;
                match.type = RuleMatch::Type::ConfigFile;
                match.value = match_json["config_file"];
                rule.match_conditions.push_back(match);
            }

            // Evidence type match
            if (match_json.contains("has_evidence_type")) {
                RuleMatch match;
                match.type = RuleMatch::Type::EvidenceType;
                match.value = match_json["has_evidence_type"];
                rule.match_conditions.push_back(match);
            }
        }

        // Parse action
        if (rule_json.contains("action")) {
            const auto& action_json = rule_json["action"];

            // Add evidence action
            if (action_json.contains("add_evidence")) {
                const auto& evidence_json = action_json["add_evidence"];
                RuleAction action;
                action.type = RuleAction::Type::AddEvidence;
                action.evidence_type = evidence_json.value("type", "custom_rule");
                action.evidence_description = evidence_json.value("description", "");
                action.evidence_confidence = evidence_json.value("confidence", 0.5);
                action.evidence_source = evidence_json.value("source", "rulepack");
                rule.actions.push_back(action);
            }

            // Boost confidence action
            if (action_json.contains("boost_confidence")) {
                RuleAction action;
                action.type = RuleAction::Type::BoostConfidence;
                action.boost_factor = action_json["boost_confidence"].get<double>();
                rule.actions.push_back(action);
            }

            // Set minimum confidence action
            if (action_json.contains("set_minimum_confidence")) {
                RuleAction action;
                action.type = RuleAction::Type::SetMinimumConfidence;
                action.minimum_confidence = action_json["set_minimum_confidence"].get<double>();
                rule.actions.push_back(action);
            }

            // Add tag action
            if (action_json.contains("add_tag")) {
                RuleAction action;
                action.type = RuleAction::Type::AddTag;
                action.tag = action_json["add_tag"].get<std::string>();
                rule.actions.push_back(action);
            }

            // NEW: Add negative evidence action
            if (action_json.contains("add_negative_evidence")) {
                const auto& neg_ev_json = action_json["add_negative_evidence"];
                RuleAction action;
                action.type = RuleAction::Type::AddNegativeEvidence;
                action.negative_evidence_type = neg_ev_json.value("type", "rulepack_negative");
                action.negative_evidence_description = neg_ev_json.value("description", "");
                action.negative_evidence_confidence = neg_ev_json.value("confidence", 0.99);
                rule.actions.push_back(action);
            }

            // NEW: Set maximum confidence action
            if (action_json.contains("set_maximum_confidence")) {
                RuleAction action;
                action.type = RuleAction::Type::SetMaximumConfidence;
                action.maximum_confidence = action_json["set_maximum_confidence"].get<double>();
                rule.actions.push_back(action);
            }

            // NEW: Exclude action
            if (action_json.contains("exclude")) {
                if (action_json["exclude"].get<bool>()) {
                    RuleAction action;
                    action.type = RuleAction::Type::Exclude;
                    rule.actions.push_back(action);
                }
            }
        }

        rulepack.rules.push_back(rule);
    }

    return rulepack;
}

void Rulepack::apply(Candidate& candidate) const {
    for (const auto& rule : rules) {
        rule.apply(candidate);
    }
}

// RuleEngine implementation
void RuleEngine::add_rulepack(const Rulepack& rulepack) {
    rulepacks_.push_back(rulepack);
}

void RuleEngine::load_rulepack(const std::string& path) {
    auto rulepack = Rulepack::load_from_file(path);
    add_rulepack(rulepack);
}

void RuleEngine::apply(Candidate& candidate) const {
    for (const auto& rulepack : rulepacks_) {
        rulepack.apply(candidate);
    }
}

} // namespace kyros
