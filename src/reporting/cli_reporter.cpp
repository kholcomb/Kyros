#include <kyros/reporting/cli_reporter.hpp>
#include <iomanip>

namespace kyros {

std::string cliReporter::file_extension() const {
    return "txt";
}

void cliReporter::generate(const ScanResults& results, std::ostream& output) {
    output << "=== Kyros MCP Server Discovery Report ===\n\n";

    // Passive scan results
    const auto& passive = results.passive_results;
    output << "Passive Scan Statistics:\n";
    output << "  Config files checked: " << passive.config_files_checked << "\n";
    output << "  Processes scanned: " << passive.processes_scanned << "\n";
    output << "  Network sockets checked: " << passive.network_sockets_checked << "\n";
    output << "  Containers scanned: " << passive.containers_scanned << "\n";
    output << "  Scan duration: " << std::fixed << std::setprecision(2)
           << passive.scan_duration_seconds << " seconds\n\n";

    output << "Candidates Found: " << passive.candidates.size() << "\n";
    for (size_t i = 0; i < passive.candidates.size(); ++i) {
        const auto& candidate = passive.candidates[i];
        output << "\n[" << (i + 1) << "] ";

        // Config candidates have priority display (they have explicit names)
        if (candidate.is_config_candidate()) {
            output << "Config: " << candidate.config_key << "\n";
            output << "    Source: " << candidate.config_file << "\n";
            if (!candidate.command.empty()) {
                output << "    Command: " << candidate.command << "\n";
            }
            if (!candidate.url.empty()) {
                output << "    URL: " << candidate.url << "\n";
            }
        } else if (candidate.is_process_candidate()) {
            output << "Process: " << candidate.process_name
                   << " (PID: " << candidate.pid << ")\n";
            output << "    Command: " << candidate.command << "\n";
        } else if (candidate.is_network_candidate()) {
            output << "Network: " << candidate.url << "\n";
            if (candidate.port > 0) {
                output << "    Port: " << candidate.port << "\n";
            }
        }

        output << "    Confidence: " << std::fixed << std::setprecision(2)
               << (candidate.confidence_score * 100) << "%\n";

        if (results.verbose && !candidate.evidence.empty()) {
            output << "    Evidence (" << candidate.evidence.size() << " items):\n";
            for (const auto& evidence : candidate.evidence) {
                output << "      [" << evidence.type << "] ";
                output << evidence.description;
                output << " (confidence: " << std::fixed << std::setprecision(1)
                       << (evidence.confidence * 100) << "%)";
                if (!evidence.source.empty()) {
                    output << "\n        Source: " << evidence.source;
                }
                output << "\n";
            }
        } else {
            output << "    Evidence count: " << candidate.evidence.size() << "\n";
        }
    }

    // Active scan results
    // Count direct detections and high-confidence passive detections
    int direct_detection_count = 0;
    int high_confidence_count = 0;
    std::vector<const Candidate*> direct_detections;
    std::vector<const Candidate*> high_confidence_candidates;

    for (const auto& candidate : results.passive_results.candidates) {
        if (candidate.is_direct_detection()) {
            direct_detection_count++;
            direct_detections.push_back(&candidate);
        } else if (candidate.confidence_score >= 0.95) {
            high_confidence_count++;
            high_confidence_candidates.push_back(&candidate);
        }
    }

    if (results.has_active_results()) {
        const auto& active = *results.active_results;
        output << "\n\n=== Active Scan Results ===\n";
        output << "Candidates tested: " << active.candidates_tested_count << "\n";
        output << "Servers confirmed: " << active.servers_confirmed_count << "\n";
        output << "Tests failed: " << active.tests_failed_count << "\n";
        output << "Scan duration: " << std::fixed << std::setprecision(2)
               << active.scan_duration_seconds << " seconds\n\n";

        // Show total detected servers (confirmed via testing + direct detection)
        // Note: Actively confirmed servers ARE direct detections (verified MCP protocol response)
        int total_confirmed = active.confirmed_servers.size() + direct_detection_count;
        output << "Total MCP Servers Confirmed: " << total_confirmed << "\n";
        output << "  - Actively Tested (stdio/HTTP/SSE): " << active.confirmed_servers.size() << "\n";
        output << "  - Direct Detection (config/extension/rulepack): " << direct_detection_count << "\n";
        if (high_confidence_count > 0) {
            output << "  - Additional High-Confidence (≥95%): " << high_confidence_count << "\n";
        }
        output << "\n";

        output << "Actively Confirmed (Tested via MCP Protocol): " << active.confirmed_servers.size() << "\n";
        for (size_t i = 0; i < active.confirmed_servers.size(); ++i) {
            const auto& server = active.confirmed_servers[i];
            output << "\n[" << (i + 1) << "] " << server.server_name
                   << " v" << server.server_version << "\n";
            output << "    Protocol: " << server.protocol_version << "\n";
            output << "    Endpoint: " << server.endpoint() << "\n";

            // Tools
            if (!server.tools.empty()) {
                output << "    Tools (" << server.tools.size() << "):\n";
                for (const auto& tool : server.tools) {
                    output << "      - " << tool.name;
                    if (!tool.description.empty()) {
                        output << ": " << tool.description;
                    }
                    output << "\n";

                    if (!tool.required_parameters.empty()) {
                        output << "        Required: ";
                        for (size_t j = 0; j < tool.required_parameters.size(); ++j) {
                            if (j > 0) output << ", ";
                            output << tool.required_parameters[j];
                        }
                        output << "\n";
                    }

                    if (!tool.optional_parameters.empty()) {
                        output << "        Optional: ";
                        for (size_t j = 0; j < tool.optional_parameters.size(); ++j) {
                            if (j > 0) output << ", ";
                            output << tool.optional_parameters[j];
                        }
                        output << "\n";
                    }
                }
            }

            // Resources
            if (!server.resources.empty()) {
                output << "    Resources (" << server.resources.size() << "):\n";
                for (const auto& resource : server.resources) {
                    output << "      - " << resource.uri;
                    if (!resource.name.empty()) {
                        output << " (" << resource.name << ")";
                    }
                    output << "\n";
                    if (!resource.description.empty()) {
                        output << "        " << resource.description << "\n";
                    }
                    if (!resource.mime_type.empty()) {
                        output << "        Type: " << resource.mime_type << "\n";
                    }
                }
            }

            // Resource Templates
            if (!server.resource_templates.empty()) {
                output << "    Resource Templates (" << server.resource_templates.size() << "):\n";
                for (const auto& tmpl : server.resource_templates) {
                    output << "      - " << tmpl.uri_template;
                    if (!tmpl.name.empty()) {
                        output << " (" << tmpl.name << ")";
                    }
                    output << "\n";
                    if (!tmpl.description.empty()) {
                        output << "        " << tmpl.description << "\n";
                    }
                    if (!tmpl.parameters.empty()) {
                        output << "        Parameters: ";
                        for (size_t j = 0; j < tmpl.parameters.size(); ++j) {
                            if (j > 0) output << ", ";
                            output << tmpl.parameters[j];
                        }
                        output << "\n";
                    }
                }
            }

            // Prompts
            if (!server.prompts.empty()) {
                output << "    Prompts (" << server.prompts.size() << "):\n";
                for (const auto& prompt : server.prompts) {
                    output << "      - " << prompt.name;
                    if (!prompt.description.empty()) {
                        output << ": " << prompt.description;
                    }
                    output << "\n";

                    if (!prompt.arguments.empty()) {
                        for (const auto& arg : prompt.arguments) {
                            output << "        ";
                            if (arg.required) {
                                output << "[required] ";
                            } else {
                                output << "[optional] ";
                            }
                            output << arg.name;
                            if (!arg.description.empty()) {
                                output << ": " << arg.description;
                            }
                            output << "\n";
                        }
                    }
                }
            }

            // Interrogation status
            if (server.interrogation_attempted && !server.interrogation_successful) {
                output << "    Interrogation: Failed\n";
                if (!server.interrogation_errors.empty()) {
                    output << "    Errors:\n";
                    for (const auto& err : server.interrogation_errors) {
                        output << "      - " << err << "\n";
                    }
                }
            }
        }

        // Show direct detections (confirmed without needing active testing)
        if (!direct_detections.empty()) {
            output << "\n\nDirect Detections (Confirmed via Config/Extension/Rulepack): " << direct_detections.size() << "\n";
            output << "(Explicitly installed/configured - confirmed MCP servers)\n";
            for (size_t i = 0; i < direct_detections.size(); ++i) {
                const auto& candidate = *direct_detections[i];
                output << "\n[" << (i + 1) << "] ";

                if (candidate.is_config_candidate()) {
                    output << "Config: " << candidate.config_key << "\n";
                    output << "    Source: " << candidate.config_file << "\n";
                } else if (candidate.pid > 0) {
                    output << "Process: " << candidate.process_name << " (PID: " << candidate.pid << ")\n";
                } else if (!candidate.url.empty()) {
                    output << "URL: " << candidate.url << "\n";
                } else {
                    output << "Unknown\n";
                }

                if (!candidate.command.empty()) {
                    output << "    Command: " << candidate.command << "\n";
                }

                output << "    Confidence: " << std::fixed << std::setprecision(2)
                       << (candidate.confidence_score * 100.0) << "%\n";

                if (results.verbose && !candidate.evidence.empty()) {
                    output << "    Evidence (" << candidate.evidence.size() << " items):\n";
                    for (const auto& evidence : candidate.evidence) {
                        output << "      [" << evidence.type << "] " << evidence.description
                               << " (confidence: " << std::fixed << std::setprecision(1)
                               << (evidence.confidence * 100.0) << "%)\n";
                        if (!evidence.source.empty()) {
                            output << "        Source: " << evidence.source << "\n";
                        }
                    }
                }
            }
        }

        // Show high-confidence detections that weren't actively confirmed
        if (!high_confidence_candidates.empty()) {
            output << "\n\nHigh-Confidence Detections (≥95%): " << high_confidence_candidates.size() << "\n";
            output << "(Not actively tested, but detected with very high confidence)\n";
            for (size_t i = 0; i < high_confidence_candidates.size(); ++i) {
                const auto& candidate = *high_confidence_candidates[i];
                output << "\n[" << (i + 1) << "] ";

                if (candidate.is_config_candidate()) {
                    output << "Config: " << candidate.config_key << "\n";
                    output << "    Source: " << candidate.config_file << "\n";
                } else if (candidate.pid > 0) {
                    output << "Process: " << candidate.process_name << " (PID: " << candidate.pid << ")\n";
                } else if (!candidate.url.empty()) {
                    output << "URL: " << candidate.url << "\n";
                } else {
                    output << "Unknown\n";
                }

                if (!candidate.command.empty()) {
                    output << "    Command: " << candidate.command << "\n";
                }

                output << "    Confidence: " << std::fixed << std::setprecision(2)
                       << (candidate.confidence_score * 100.0) << "%\n";

                if (results.verbose && !candidate.evidence.empty()) {
                    output << "    Evidence (" << candidate.evidence.size() << " items):\n";
                    for (const auto& evidence : candidate.evidence) {
                        output << "      [" << evidence.type << "] " << evidence.description
                               << " (confidence: " << std::fixed << std::setprecision(1)
                               << (evidence.confidence * 100.0) << "%)\n";
                        if (!evidence.source.empty()) {
                            output << "        Source: " << evidence.source << "\n";
                        }
                    }
                }
            }
        }
    } else {
        // Passive-only mode - show direct detections and high-confidence as main output
        int total_detected = direct_detection_count + high_confidence_count;
        if (total_detected > 0) {
            output << "\n\n=== Detected MCP Servers ===\n";
            output << "Total: " << total_detected << "\n";
            output << "  - Direct Detection (config/extension/rulepack): " << direct_detection_count << "\n";
            if (high_confidence_count > 0) {
                output << "  - High-Confidence (≥95%): " << high_confidence_count << "\n";
            }
        }

        // Show direct detections first
        if (!direct_detections.empty()) {
            output << "\n\n=== Direct Detections (Confirmed MCP Servers) ===\n";
            for (size_t i = 0; i < direct_detections.size(); ++i) {
                const auto& candidate = *direct_detections[i];
                output << "\n[" << (i + 1) << "] ";

                if (candidate.is_config_candidate()) {
                    output << "Config: " << candidate.config_key << "\n";
                    output << "    Source: " << candidate.config_file << "\n";
                } else if (candidate.pid > 0) {
                    output << "Process: " << candidate.process_name << " (PID: " << candidate.pid << ")\n";
                } else if (!candidate.url.empty()) {
                    output << "URL: " << candidate.url << "\n";
                } else {
                    output << "Unknown\n";
                }

                if (!candidate.command.empty()) {
                    output << "    Command: " << candidate.command << "\n";
                }

                output << "    Confidence: " << std::fixed << std::setprecision(2)
                       << (candidate.confidence_score * 100.0) << "%\n";

                if (results.verbose && !candidate.evidence.empty()) {
                    output << "    Evidence (" << candidate.evidence.size() << " items):\n";
                    for (const auto& evidence : candidate.evidence) {
                        output << "      [" << evidence.type << "] " << evidence.description
                               << " (confidence: " << std::fixed << std::setprecision(1)
                               << (evidence.confidence * 100.0) << "%)\n";
                        if (!evidence.source.empty()) {
                            output << "        Source: " << evidence.source << "\n";
                        }
                    }
                }
            }
        }

        // Show high-confidence detections
        if (!high_confidence_candidates.empty()) {
            output << "\n\n=== High-Confidence Detections (≥95%) ===\n";
            for (size_t i = 0; i < high_confidence_candidates.size(); ++i) {
                const auto& candidate = *high_confidence_candidates[i];
                output << "\n[" << (i + 1) << "] ";

                if (candidate.is_config_candidate()) {
                    output << "Config: " << candidate.config_key << "\n";
                    output << "    Source: " << candidate.config_file << "\n";
                } else if (candidate.pid > 0) {
                    output << "Process: " << candidate.process_name << " (PID: " << candidate.pid << ")\n";
                } else if (!candidate.url.empty()) {
                    output << "URL: " << candidate.url << "\n";
                } else {
                    output << "Unknown\n";
                }

                if (!candidate.command.empty()) {
                    output << "    Command: " << candidate.command << "\n";
                }

                output << "    Confidence: " << std::fixed << std::setprecision(2)
                       << (candidate.confidence_score * 100.0) << "%\n";

                if (results.verbose && !candidate.evidence.empty()) {
                    output << "    Evidence (" << candidate.evidence.size() << " items):\n";
                    for (const auto& evidence : candidate.evidence) {
                        output << "      [" << evidence.type << "] " << evidence.description
                               << " (confidence: " << std::fixed << std::setprecision(1)
                               << (evidence.confidence * 100.0) << "%)\n";
                        if (!evidence.source.empty()) {
                            output << "        Source: " << evidence.source << "\n";
                        }
                    }
                }
            }
        }
    }

    // Errors
    if (!results.errors.empty()) {
        output << "\n\n=== Errors ===\n";
        for (const auto& error : results.errors) {
            output << "  - " << error << "\n";
        }
    }

    output << "\n";
}

} // namespace kyros
