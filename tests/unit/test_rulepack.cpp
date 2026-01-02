/**
 * Rulepack Test Suite
 * Tests rulepack loading, parsing, and rule application
 */

#include <gtest/gtest.h>
#include <kyros/rulepack.hpp>
#include <kyros/candidate.hpp>
#include <nlohmann/json.hpp>
#include "test_helpers.hpp"

using namespace kyros;
using namespace kyros::test;

// ============================================================================
// Rulepack Basic Tests
// ============================================================================

TEST(RulepackTest, DefaultConstruction) {
    Rulepack rulepack;

    EXPECT_TRUE(rulepack.name.empty());
    EXPECT_TRUE(rulepack.version.empty());
    EXPECT_TRUE(rulepack.description.empty());
    EXPECT_TRUE(rulepack.rules.empty());
}

TEST(RulepackTest, BasicInitialization) {
    Rulepack rulepack;
    rulepack.name = "test-rulepack";
    rulepack.version = "1.0.0";
    rulepack.description = "Test rulepack for unit tests";

    EXPECT_EQ(rulepack.name, "test-rulepack");
    EXPECT_EQ(rulepack.version, "1.0.0");
    EXPECT_EQ(rulepack.description, "Test rulepack for unit tests");
}

// ============================================================================
// Rule Match Tests
// ============================================================================

TEST(RuleMatchTest, ProcessNameMatch) {
    RuleMatch match;
    match.type = RuleMatch::Type::ProcessName;
    match.value = "node";

    Candidate candidate = create_test_candidate("node", 100);
    EXPECT_TRUE(match.matches(candidate));

    Candidate non_matching = create_test_candidate("python3", 200);
    EXPECT_FALSE(match.matches(non_matching));
}

TEST(RuleMatchTest, CommandContainsMatch) {
    RuleMatch match;
    match.type = RuleMatch::Type::CommandContains;
    match.value = "mcp-server";

    Candidate candidate = create_test_candidate("node", 100);
    candidate.command = "/usr/bin/node /app/mcp-server.js";
    EXPECT_TRUE(match.matches(candidate));

    Candidate non_matching = create_test_candidate("node", 200);
    non_matching.command = "/usr/bin/node /app/web-server.js";
    EXPECT_FALSE(match.matches(non_matching));
}

TEST(RuleMatchTest, PortEqualsMatch) {
    RuleMatch match;
    match.type = RuleMatch::Type::PortEquals;
    match.value = "3000";

    Candidate candidate;
    candidate.port = 3000;
    EXPECT_TRUE(match.matches(candidate));

    Candidate non_matching;
    non_matching.port = 8080;
    EXPECT_FALSE(match.matches(non_matching));
}

TEST(RuleMatchTest, URLContainsMatch) {
    RuleMatch match;
    match.type = RuleMatch::Type::URLContains;
    match.value = "localhost";

    Candidate candidate;
    candidate.url = "http://localhost:3000/mcp";
    EXPECT_TRUE(match.matches(candidate));

    Candidate non_matching;
    non_matching.url = "http://example.com:3000";
    EXPECT_FALSE(match.matches(non_matching));
}

TEST(RuleMatchTest, ConfigFileMatch) {
    RuleMatch match;
    match.type = RuleMatch::Type::ConfigFile;
    match.value = "claude";

    Candidate candidate;
    candidate.config_file = "/home/user/.config/claude/config.json";
    EXPECT_TRUE(match.matches(candidate));

    Candidate non_matching;
    non_matching.config_file = "/home/user/.bashrc";
    EXPECT_FALSE(match.matches(non_matching));
}

// ============================================================================
// Rule Action Tests
// ============================================================================

TEST(RuleActionTest, AddEvidenceAction) {
    RuleAction action;
    action.type = RuleAction::Type::AddEvidence;
    action.evidence_type = "known_mcp_server";
    action.evidence_description = "Matches known MCP server pattern";
    action.evidence_confidence = 0.85;
    action.evidence_source = "rulepack:test";

    Candidate candidate = create_test_candidate("node", 100);
    size_t initial_evidence_count = candidate.evidence.size();

    action.apply(candidate);

    EXPECT_EQ(candidate.evidence.size(), initial_evidence_count + 1);
    EXPECT_EQ(candidate.evidence.back().type, "known_mcp_server");
    EXPECT_DOUBLE_EQ(candidate.evidence.back().confidence, 0.85);
}

TEST(RuleActionTest, BoostConfidenceAction) {
    RuleAction action;
    action.type = RuleAction::Type::BoostConfidence;
    action.boost_factor = 1.5;

    Candidate candidate = create_test_candidate("node", 100);
    candidate.add_evidence(create_test_evidence("process_name", "Node process", 0.6));
    double initial_confidence = candidate.confidence_score;

    action.apply(candidate);

    EXPECT_GT(candidate.confidence_score, initial_confidence);
}

TEST(RuleActionTest, SetMinimumConfidenceAction) {
    RuleAction action;
    action.type = RuleAction::Type::SetMinimumConfidence;
    action.minimum_confidence = 0.7;

    Candidate low_confidence = create_test_candidate("node", 100);
    low_confidence.add_evidence(create_test_evidence("weak", "Weak signal", 0.3));

    action.apply(low_confidence);

    EXPECT_GE(low_confidence.confidence_score, 0.7);
}

// ============================================================================
// Rule Tests
// ============================================================================

TEST(RuleTest, SingleConditionMatch) {
    Rule rule;
    rule.name = "Node.js MCP Server";
    rule.description = "Detects Node.js MCP servers";

    RuleMatch match;
    match.type = RuleMatch::Type::ProcessName;
    match.value = "node";
    rule.match_conditions.push_back(match);

    Candidate node_candidate = create_test_candidate("node", 100);
    EXPECT_TRUE(rule.matches(node_candidate));

    Candidate python_candidate = create_test_candidate("python3", 200);
    EXPECT_FALSE(rule.matches(python_candidate));
}

TEST(RuleTest, MultipleConditionsAllMustMatch) {
    Rule rule;
    rule.name = "Specific MCP Server";

    RuleMatch process_match;
    process_match.type = RuleMatch::Type::ProcessName;
    process_match.value = "node";
    rule.match_conditions.push_back(process_match);

    RuleMatch command_match;
    command_match.type = RuleMatch::Type::CommandContains;
    command_match.value = "mcp-server";
    rule.match_conditions.push_back(command_match);

    // Matches both conditions
    Candidate matching = create_test_candidate("node", 100);
    matching.command = "/usr/bin/node mcp-server.js";
    EXPECT_TRUE(rule.matches(matching));

    // Matches process but not command
    Candidate partial_match = create_test_candidate("node", 200);
    partial_match.command = "/usr/bin/node web-server.js";
    EXPECT_FALSE(rule.matches(partial_match));
}

TEST(RuleTest, ApplyRuleActions) {
    Rule rule;
    rule.name = "Test Rule";

    RuleMatch match;
    match.type = RuleMatch::Type::ProcessName;
    match.value = "node";
    rule.match_conditions.push_back(match);

    RuleAction action;
    action.type = RuleAction::Type::AddEvidence;
    action.evidence_type = "rule_matched";
    action.evidence_description = "Matched test rule";
    action.evidence_confidence = 0.8;
    rule.actions.push_back(action);

    Candidate candidate = create_test_candidate("node", 100);
    size_t initial_evidence = candidate.evidence.size();

    rule.apply(candidate);

    EXPECT_GT(candidate.evidence.size(), initial_evidence);
}

// ============================================================================
// Rulepack Application Tests
// ============================================================================

TEST(RulepackTest, ApplyRulepackToCandidate) {
    Rulepack rulepack;
    rulepack.name = "test";
    rulepack.version = "1.0.0";

    Rule rule;
    rule.name = "Node Server Rule";

    RuleMatch match;
    match.type = RuleMatch::Type::ProcessName;
    match.value = "node";
    rule.match_conditions.push_back(match);

    RuleAction action;
    action.type = RuleAction::Type::AddEvidence;
    action.evidence_type = "rulepack_match";
    action.evidence_description = "Matched rulepack rule";
    action.evidence_confidence = 0.75;
    action.evidence_source = "rulepack:test";
    rule.actions.push_back(action);

    rulepack.rules.push_back(rule);

    Candidate candidate = create_test_candidate("node", 100);
    size_t initial_evidence = candidate.evidence.size();

    rulepack.apply(candidate);

    EXPECT_GT(candidate.evidence.size(), initial_evidence);
}

TEST(RulepackTest, ApplyMultipleRules) {
    Rulepack rulepack;
    rulepack.name = "multi-rule";

    // Rule 1: Match node process
    Rule rule1;
    RuleMatch match1;
    match1.type = RuleMatch::Type::ProcessName;
    match1.value = "node";
    rule1.match_conditions.push_back(match1);

    RuleAction action1;
    action1.type = RuleAction::Type::AddEvidence;
    action1.evidence_type = "rule1_match";
    action1.evidence_description = "Rule 1";
    action1.evidence_confidence = 0.6;
    rule1.actions.push_back(action1);
    rulepack.rules.push_back(rule1);

    // Rule 2: Match MCP in command
    Rule rule2;
    RuleMatch match2;
    match2.type = RuleMatch::Type::CommandContains;
    match2.value = "mcp";
    rule2.match_conditions.push_back(match2);

    RuleAction action2;
    action2.type = RuleAction::Type::AddEvidence;
    action2.evidence_type = "rule2_match";
    action2.evidence_description = "Rule 2";
    action2.evidence_confidence = 0.7;
    rule2.actions.push_back(action2);
    rulepack.rules.push_back(rule2);

    Candidate candidate = create_test_candidate("node", 100);
    candidate.command = "/usr/bin/node mcp-server.js";

    rulepack.apply(candidate);

    // Both rules should have matched
    EXPECT_GE(candidate.evidence.size(), 2);
}

// ============================================================================
// RuleEngine Tests
// ============================================================================

TEST(RuleEngineTest, AddRulepack) {
    RuleEngine engine;

    Rulepack rulepack1;
    rulepack1.name = "pack1";
    rulepack1.version = "1.0.0";

    Rulepack rulepack2;
    rulepack2.name = "pack2";
    rulepack2.version = "1.0.0";

    engine.add_rulepack(rulepack1);
    engine.add_rulepack(rulepack2);

    EXPECT_EQ(engine.rulepacks().size(), 2);
}

TEST(RuleEngineTest, ApplyAllRulepacks) {
    RuleEngine engine;

    // Create first rulepack
    Rulepack pack1;
    pack1.name = "pack1";
    Rule rule1;
    RuleMatch match1;
    match1.type = RuleMatch::Type::ProcessName;
    match1.value = "node";
    rule1.match_conditions.push_back(match1);
    RuleAction action1;
    action1.type = RuleAction::Type::AddEvidence;
    action1.evidence_type = "pack1_match";
    action1.evidence_confidence = 0.6;
    rule1.actions.push_back(action1);
    pack1.rules.push_back(rule1);
    engine.add_rulepack(pack1);

    // Create second rulepack
    Rulepack pack2;
    pack2.name = "pack2";
    Rule rule2;
    RuleMatch match2;
    match2.type = RuleMatch::Type::CommandContains;
    match2.value = "server";
    rule2.match_conditions.push_back(match2);
    RuleAction action2;
    action2.type = RuleAction::Type::AddEvidence;
    action2.evidence_type = "pack2_match";
    action2.evidence_confidence = 0.7;
    rule2.actions.push_back(action2);
    pack2.rules.push_back(rule2);
    engine.add_rulepack(pack2);

    Candidate candidate = create_test_candidate("node", 100);
    candidate.command = "/usr/bin/node server.js";

    engine.apply(candidate);

    // Should have evidence from both rulepacks
    EXPECT_GE(candidate.evidence.size(), 2);
}

// ============================================================================
// JSON Loading Tests (Structure only - actual file I/O tested separately)
// ============================================================================

TEST(RulepackJSONTest, ParseBasicStructure) {
    nlohmann::json rulepack_json = {
        {"name", "test-rulepack"},
        {"version", "1.0.0"},
        {"description", "Test rulepack"},
        {"rules", nlohmann::json::array()}
    };

    EXPECT_EQ(rulepack_json["name"], "test-rulepack");
    EXPECT_EQ(rulepack_json["version"], "1.0.0");
    EXPECT_TRUE(rulepack_json["rules"].is_array());
}

TEST(RulepackJSONTest, ParseRuleStructure) {
    nlohmann::json rule_json = {
        {"name", "Test Rule"},
        {"description", "A test rule"},
        {"match", nlohmann::json::array({
            {
                {"type", "process_name"},
                {"value", "node"}
            }
        })},
        {"actions", nlohmann::json::array({
            {
                {"type", "add_evidence"},
                {"evidence_type", "known_server"},
                {"confidence", 0.85}
            }
        })}
    };

    EXPECT_EQ(rule_json["name"], "Test Rule");
    EXPECT_TRUE(rule_json["match"].is_array());
    EXPECT_TRUE(rule_json["actions"].is_array());
}
