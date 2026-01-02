/**
 * Detection Engines Test Suite
 * Tests for detection engine structures and basic functionality
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <kyros/detection/detection_engine.hpp>
#include <kyros/candidate.hpp>
#include <kyros/rulepack.hpp>
#include "test_helpers.hpp"

using namespace kyros;
using namespace kyros::test;

// ============================================================================
// Detection Engine Basic Tests
// ============================================================================

TEST(DetectionEngineTest, CandidateStructure) {
    Candidate candidate = create_test_candidate("node", 12345);

    EXPECT_EQ(candidate.process_name, "node");
    EXPECT_EQ(candidate.pid, 12345);
    EXPECT_FALSE(candidate.command.empty());
}

TEST(DetectionEngineTest, CandidateCommandField) {
    Candidate candidate;
    candidate.process_name = "python3";
    candidate.pid = 54321;
    candidate.command = "/usr/bin/python3 /app/mcp_server.py";

    EXPECT_EQ(candidate.command, "/usr/bin/python3 /app/mcp_server.py");
    EXPECT_TRUE(candidate.command.find("mcp_server") != std::string::npos);
}

TEST(DetectionEngineTest, CandidateNetworkFields) {
    Candidate candidate;
    candidate.url = "http://localhost:3000";
    candidate.port = 3000;
    candidate.address = "127.0.0.1";

    EXPECT_EQ(candidate.url, "http://localhost:3000");
    EXPECT_EQ(candidate.port, 3000);
    EXPECT_TRUE(candidate.is_network_candidate());
}

TEST(DetectionEngineTest, CandidateConfigFields) {
    Candidate candidate;
    candidate.config_file = "/home/user/.config/mcp/servers.json";
    candidate.config_key = "filesystem";

    EXPECT_TRUE(candidate.is_config_candidate());
    EXPECT_FALSE(candidate.is_process_candidate());
}

// ============================================================================
// Evidence and Confidence Tests
// ============================================================================

TEST(DetectionEngineTest, AddEvidenceToCandidate) {
    Candidate candidate = create_test_candidate("node", 100);

    candidate.add_evidence(create_test_evidence(
        "process_name",
        "Node.js process detected",
        0.7
    ));

    EXPECT_EQ(candidate.evidence.size(), 1);
    EXPECT_GT(candidate.confidence_score, 0.0);
}

TEST(DetectionEngineTest, MultipleEvidenceAccumulation) {
    Candidate candidate = create_test_candidate("node", 100);

    candidate.add_evidence(create_test_evidence("process_name", "Node detected", 0.6));
    candidate.add_evidence(create_test_evidence("command", "MCP in command", 0.8));
    candidate.add_evidence(create_test_evidence("config", "In config file", 0.9));

    EXPECT_EQ(candidate.evidence.size(), 3);
    EXPECT_GT(candidate.confidence_score, 0.6);
}

// ============================================================================
// Transport Type Tests
// ============================================================================

TEST(DetectionEngineTest, TransportTypeHints) {
    Candidate stdio_candidate = create_test_candidate("node", 100);
    stdio_candidate.transport_hint = TransportType::Stdio;

    Candidate http_candidate;
    http_candidate.url = "http://localhost:3000";
    http_candidate.transport_hint = TransportType::Http;

    EXPECT_EQ(stdio_candidate.transport_hint, TransportType::Stdio);
    EXPECT_EQ(http_candidate.transport_hint, TransportType::Http);
}

// ============================================================================
// Container Detection Tests
// ============================================================================

TEST(DetectionEngineTest, DockerContainerCandidate) {
    Candidate candidate;

    DockerContainer container;
    container.id = "abc123def456";
    container.name = "mcp-server";
    container.image = "mcp/server:latest";
    container.command = "/app/server.sh";

    candidate.docker_container = container;

    EXPECT_TRUE(candidate.is_container_candidate());
    EXPECT_EQ(candidate.docker_container->name, "mcp-server");
}

TEST(DetectionEngineTest, KubernetesPodCandidate) {
    Candidate candidate;

    KubernetesPod pod;
    pod.name = "mcp-server-pod";
    pod.namespace_name = "default";
    pod.pod_ip = "10.0.0.1";

    candidate.k8s_pod = pod;

    EXPECT_TRUE(candidate.is_container_candidate());
    EXPECT_EQ(candidate.k8s_pod->name, "mcp-server-pod");
}

// ============================================================================
// Candidate Type Classification Tests
// ============================================================================

TEST(DetectionEngineTest, CandidateTypeChecks) {
    // Process candidate
    Candidate process_cand = create_test_candidate("node", 100);
    EXPECT_TRUE(process_cand.is_process_candidate());
    EXPECT_FALSE(process_cand.is_network_candidate());
    EXPECT_FALSE(process_cand.is_config_candidate());

    // Network candidate
    Candidate network_cand;
    network_cand.port = 3000;
    EXPECT_TRUE(network_cand.is_network_candidate());
    EXPECT_FALSE(network_cand.is_process_candidate());

    // Config candidate
    Candidate config_cand;
    config_cand.config_file = "/path/to/config.json";
    EXPECT_TRUE(config_cand.is_config_candidate());
    EXPECT_FALSE(config_cand.is_process_candidate());
}

// ============================================================================
// Direct Detection Tests
// ============================================================================

TEST(DetectionEngineTest, DirectDetectionIndicators) {
    Candidate candidate = create_test_candidate("node", 100);
    EXPECT_FALSE(candidate.is_direct_detection());

    // Claude extension installed
    candidate.add_evidence(create_test_evidence(
        "claude_extension_installed",
        "Installed by Claude Desktop",
        0.95
    ));
    EXPECT_TRUE(candidate.is_direct_detection());
}

TEST(DetectionEngineTest, RulepackSourceIndicatesDirectDetection) {
    Candidate candidate = create_test_candidate("node", 100);

    candidate.add_evidence(create_test_evidence(
        "known_pattern",
        "Matches known pattern",
        0.85,
        "rulepack:default"  // Rulepack source = direct detection
    ));

    EXPECT_TRUE(candidate.is_direct_detection());
}
