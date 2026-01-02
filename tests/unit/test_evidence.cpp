/**
 * Evidence Test Suite
 * Tests evidence collection, scoring, and aggregation
 */

#include <gtest/gtest.h>
#include <kyros/evidence.hpp>
#include <kyros/candidate.hpp>
#include "test_helpers.hpp"

using namespace kyros;
using namespace kyros::test;

// ============================================================================
// Evidence Creation Tests
// ============================================================================

TEST(EvidenceTest, DefaultConstruction) {
    Evidence evidence;

    EXPECT_TRUE(evidence.type.empty());
    EXPECT_TRUE(evidence.description.empty());
    EXPECT_DOUBLE_EQ(evidence.confidence, 0.0);
    EXPECT_TRUE(evidence.source.empty());
}

TEST(EvidenceTest, ParameterizedConstruction) {
    Evidence evidence("process_name", "Node.js process detected", 0.8, "pid:12345");

    EXPECT_EQ(evidence.type, "process_name");
    EXPECT_EQ(evidence.description, "Node.js process detected");
    EXPECT_DOUBLE_EQ(evidence.confidence, 0.8);
    EXPECT_EQ(evidence.source, "pid:12345");
}

TEST(EvidenceTest, CreateViaHelper) {
    auto evidence = create_test_evidence("cmdline", "MCP server in command line", 0.9, "test");

    EXPECT_EQ(evidence.type, "cmdline");
    EXPECT_EQ(evidence.description, "MCP server in command line");
    EXPECT_DOUBLE_EQ(evidence.confidence, 0.9);
    EXPECT_EQ(evidence.source, "test");
}

// ============================================================================
// Evidence Confidence Range Tests
// ============================================================================

TEST(EvidenceTest, ConfidenceWithinRange) {
    auto low_evidence = create_test_evidence("test", "Low confidence", 0.1);
    auto mid_evidence = create_test_evidence("test", "Mid confidence", 0.5);
    auto high_evidence = create_test_evidence("test", "High confidence", 0.95);

    EXPECT_GE(low_evidence.confidence, 0.0);
    EXPECT_LE(low_evidence.confidence, 1.0);

    EXPECT_GE(mid_evidence.confidence, 0.0);
    EXPECT_LE(mid_evidence.confidence, 1.0);

    EXPECT_GE(high_evidence.confidence, 0.0);
    EXPECT_LE(high_evidence.confidence, 1.0);
}

// ============================================================================
// Evidence Types Tests
// ============================================================================

TEST(EvidenceTest, ProcessNameEvidence) {
    Evidence evidence("process_name", "Node process detected", 0.7, "process_scan");

    EXPECT_EQ(evidence.type, "process_name");
    EXPECT_FALSE(evidence.description.empty());
}

TEST(EvidenceTest, CommandLineEvidence) {
    Evidence evidence("cmdline", "Contains MCP server indicators", 0.8, "process_scan");

    EXPECT_EQ(evidence.type, "cmdline");
    EXPECT_EQ(evidence.description, "Contains MCP server indicators");
}

TEST(EvidenceTest, ConfigFileEvidence) {
    Evidence evidence("config_file", "Declared in Claude config", 0.95, "config_scan");

    EXPECT_EQ(evidence.type, "config_file");
    EXPECT_GT(evidence.confidence, 0.9);
}

TEST(EvidenceTest, NetworkPortEvidence) {
    Evidence evidence("listening_port", "Listening on port 3000", 0.6, "network_scan");

    EXPECT_EQ(evidence.type, "listening_port");
    EXPECT_EQ(evidence.source, "network_scan");
}

// ============================================================================
// Candidate Tests
// ============================================================================

TEST(CandidateTest, DefaultConstruction) {
    Candidate candidate;

    EXPECT_EQ(candidate.pid, 0);
    EXPECT_TRUE(candidate.process_name.empty());
    EXPECT_TRUE(candidate.command.empty());
    EXPECT_TRUE(candidate.evidence.empty());
    EXPECT_DOUBLE_EQ(candidate.confidence_score, 0.0);
}

TEST(CandidateTest, CreateViaHelper) {
    auto candidate = create_test_candidate("node", 12345);

    EXPECT_EQ(candidate.process_name, "node");
    EXPECT_EQ(candidate.pid, 12345);
    EXPECT_FALSE(candidate.command.empty());
}

TEST(CandidateTest, AddSingleEvidence) {
    Candidate candidate = create_test_candidate("python3", 54321);
    Evidence evidence = create_test_evidence("process_name", "Python process", 0.7);

    candidate.add_evidence(evidence);

    ASSERT_EQ(candidate.evidence.size(), 1);
    EXPECT_EQ(candidate.evidence[0].type, "process_name");
    EXPECT_GT(candidate.confidence_score, 0.0);
}

TEST(CandidateTest, AddMultipleEvidence) {
    Candidate candidate = create_test_candidate("node", 100);

    candidate.add_evidence(create_test_evidence("process_name", "Node detected", 0.6));
    candidate.add_evidence(create_test_evidence("cmdline", "MCP in cmdline", 0.8));
    candidate.add_evidence(create_test_evidence("config_file", "In config", 0.9));

    EXPECT_EQ(candidate.evidence.size(), 3);
    EXPECT_GT(candidate.confidence_score, 0.0);
}

TEST(CandidateTest, ConfidenceRecalculation) {
    Candidate candidate = create_test_candidate("test", 999);

    // Initially zero confidence
    EXPECT_DOUBLE_EQ(candidate.confidence_score, 0.0);

    // Add evidence and check confidence increases
    candidate.add_evidence(create_test_evidence("type1", "desc1", 0.5));
    double first_confidence = candidate.confidence_score;
    EXPECT_GT(first_confidence, 0.0);

    candidate.add_evidence(create_test_evidence("type2", "desc2", 0.8));
    EXPECT_GT(candidate.confidence_score, first_confidence);
}

// ============================================================================
// Candidate Helper Method Tests
// ============================================================================

TEST(CandidateTest, IsProcessCandidate) {
    Candidate candidate = create_test_candidate("node", 100);

    EXPECT_TRUE(candidate.is_process_candidate());
    EXPECT_FALSE(candidate.is_network_candidate());
    EXPECT_FALSE(candidate.is_config_candidate());
}

TEST(CandidateTest, IsNetworkCandidate) {
    Candidate candidate;
    candidate.url = "http://localhost:3000";
    candidate.port = 3000;

    EXPECT_TRUE(candidate.is_network_candidate());
    EXPECT_FALSE(candidate.is_process_candidate());
}

TEST(CandidateTest, IsConfigCandidate) {
    Candidate candidate;
    candidate.config_file = "/home/user/.config/claude/config.json";
    candidate.config_key = "mcpServers.filesystem";

    EXPECT_TRUE(candidate.is_config_candidate());
    EXPECT_FALSE(candidate.is_process_candidate());
}

TEST(CandidateTest, IsContainerCandidate) {
    Candidate candidate;

    EXPECT_FALSE(candidate.is_container_candidate());

    // Add Docker container
    DockerContainer container;
    container.id = "abc123";
    container.name = "mcp-server";
    candidate.docker_container = container;

    EXPECT_TRUE(candidate.is_container_candidate());
}

TEST(CandidateTest, IsDirectDetection) {
    Candidate candidate = create_test_candidate("node", 100);

    // Initially not a direct detection
    EXPECT_FALSE(candidate.is_direct_detection());

    // Add claude_extension_installed evidence
    candidate.add_evidence(create_test_evidence(
        "claude_extension_installed",
        "Installed by Claude Desktop",
        0.95
    ));

    EXPECT_TRUE(candidate.is_direct_detection());
}

TEST(CandidateTest, IsDirectDetectionConfigDeclared) {
    Candidate candidate;
    candidate.add_evidence(create_test_evidence(
        "config_declared",
        "Declared in config file",
        0.95
    ));

    EXPECT_TRUE(candidate.is_direct_detection());
}

TEST(CandidateTest, IsDirectDetectionRulepack) {
    Candidate candidate;
    candidate.add_evidence(create_test_evidence(
        "known_pattern",
        "Matches known MCP server pattern",
        0.85,
        "rulepack:default"
    ));

    EXPECT_TRUE(candidate.is_direct_detection());
}

// ============================================================================
// Evidence Aggregation Tests
// ============================================================================

TEST(EvidenceAggregationTest, MultipleEvidenceTypes) {
    std::vector<Evidence> evidence_list = {
        create_test_evidence("process_name", "Node.js", 0.7),
        create_test_evidence("cmdline", "MCP server", 0.8),
        create_test_evidence("listening_port", "Port 3000", 0.6)
    };

    auto candidate = create_candidate_with_evidence("node", 100, evidence_list);

    EXPECT_EQ(candidate.evidence.size(), 3);
    EXPECT_GT(candidate.confidence_score, 0.0);
}

TEST(EvidenceAggregationTest, HighConfidenceEvidence) {
    Candidate candidate = create_test_candidate("node", 100);
    candidate.add_evidence(create_test_evidence(
        "config_declared",
        "Explicitly configured",
        0.95
    ));

    EXPECT_GT(candidate.confidence_score, 0.9);
}

TEST(EvidenceAggregationTest, LowConfidenceEvidence) {
    Candidate candidate = create_test_candidate("generic", 200);
    candidate.add_evidence(create_test_evidence(
        "process_name",
        "Generic process name",
        0.2
    ));

    EXPECT_LT(candidate.confidence_score, 0.5);
}

// ============================================================================
// Candidate Metadata Tests
// ============================================================================

TEST(CandidateMetadataTest, TransportHint) {
    Candidate candidate = create_test_candidate("node", 100);

    EXPECT_EQ(candidate.transport_hint, TransportType::Unknown);

    candidate.transport_hint = TransportType::Stdio;
    EXPECT_EQ(candidate.transport_hint, TransportType::Stdio);
}

TEST(CandidateMetadataTest, EnvironmentVariables) {
    Candidate candidate = create_test_candidate("node", 100);
    candidate.environment["MCP_SERVER"] = "true";
    candidate.environment["PORT"] = "3000";

    EXPECT_EQ(candidate.environment.size(), 2);
    EXPECT_EQ(candidate.environment["MCP_SERVER"], "true");
}

TEST(CandidateMetadataTest, ParentProcess) {
    Candidate candidate = create_test_candidate("node", 100);
    candidate.parent_pid = 50;

    EXPECT_EQ(candidate.parent_pid, 50);
    EXPECT_EQ(candidate.pid, 100);
}
