#include <kyros/reporting/html_reporter.hpp>
#include <iomanip>

namespace kyros {

std::string htmlReporter::file_extension() const {
    return "html";
}

void htmlReporter::generate(const ScanResults& results, std::ostream& output) {
    output << R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Kyros MCP Server Discovery Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; }
        h1 { color: #333; border-bottom: 2px solid #007bff; padding-bottom: 10px; }
        h2 { color: #555; margin-top: 30px; }
        .stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 20px 0; }
        .stat-box { background: #f8f9fa; padding: 15px; border-left: 4px solid #007bff; border-radius: 4px; }
        .stat-label { font-size: 12px; color: #666; text-transform: uppercase; }
        .stat-value { font-size: 24px; font-weight: bold; color: #333; margin-top: 5px; }
        .candidate { background: #fff; border: 1px solid #ddd; padding: 15px; margin: 10px 0; border-radius: 4px; }
        .candidate-title { font-weight: bold; color: #007bff; margin-bottom: 10px; }
        .server { background: #e7f3ff; border: 2px solid #007bff; padding: 15px; margin: 10px 0; border-radius: 4px; }
        .server-title { font-weight: bold; color: #0056b3; font-size: 18px; margin-bottom: 10px; }
        .detail { margin: 5px 0; color: #555; }
        .error { background: #fff3cd; border-left: 4px solid #ffc107; padding: 10px; margin: 5px 0; }
        .capability-section { margin: 15px 0 10px 0; font-weight: bold; color: #333; }
        .capability-item { background: #f8f9fa; padding: 10px; margin: 5px 0; border-left: 3px solid #28a745; border-radius: 3px; }
        .capability-name { font-weight: bold; color: #28a745; }
        .capability-desc { color: #666; margin-top: 5px; font-size: 14px; }
        .param-list { margin: 8px 0; font-size: 14px; }
        .param-badge { display: inline-block; padding: 2px 8px; margin: 2px; border-radius: 3px; font-size: 12px; font-weight: bold; }
        .param-required { background: #dc3545; color: white; }
        .param-optional { background: #6c757d; color: white; }
        .evidence-list { margin: 10px 0; padding: 10px; background: #f8f9fa; border-radius: 4px; }
        .evidence-item { margin: 5px 0; padding: 8px; background: white; border-left: 3px solid #17a2b8; border-radius: 3px; font-size: 14px; }
        .evidence-type { font-weight: bold; color: #17a2b8; }
        .evidence-confidence { color: #666; font-size: 12px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Kyros MCP Server Discovery Report</h1>
)";

    // Passive scan statistics
    const auto& passive = results.passive_results;
    output << "        <h2>Passive Scan Statistics</h2>\n";
    output << "        <div class=\"stats\">\n";
    output << "            <div class=\"stat-box\"><div class=\"stat-label\">Config Files</div>"
           << "<div class=\"stat-value\">" << passive.config_files_checked << "</div></div>\n";
    output << "            <div class=\"stat-box\"><div class=\"stat-label\">Processes</div>"
           << "<div class=\"stat-value\">" << passive.processes_scanned << "</div></div>\n";
    output << "            <div class=\"stat-box\"><div class=\"stat-label\">Network Sockets</div>"
           << "<div class=\"stat-value\">" << passive.network_sockets_checked << "</div></div>\n";
    output << "            <div class=\"stat-box\"><div class=\"stat-label\">Scan Duration</div>"
           << "<div class=\"stat-value\">" << std::fixed << std::setprecision(2)
           << passive.scan_duration_seconds << "s</div></div>\n";
    output << "        </div>\n";

    // Candidates
    output << "        <h2>Candidates Found (" << passive.candidates.size() << ")</h2>\n";
    for (size_t i = 0; i < passive.candidates.size(); ++i) {
        const auto& c = passive.candidates[i];
        output << "        <div class=\"candidate\">\n";
        output << "            <div class=\"candidate-title\">[" << (i + 1) << "] ";
        if (c.is_process_candidate()) {
            output << c.process_name << " (PID " << c.pid << ")";
        } else if (c.is_network_candidate()) {
            output << c.url;
        }
        output << "</div>\n";
        if (!c.command.empty()) {
            output << "            <div class=\"detail\">Command: " << c.command << "</div>\n";
        }
        output << "            <div class=\"detail\">Confidence: " << std::fixed << std::setprecision(1)
               << (c.confidence_score * 100) << "%</div>\n";

        if (!c.evidence.empty()) {
            output << "            <div class=\"evidence-list\">\n";
            output << "                <strong>Evidence (" << c.evidence.size() << " items):</strong>\n";
            for (const auto& evidence : c.evidence) {
                output << "                <div class=\"evidence-item\">\n";
                output << "                    <span class=\"evidence-type\">[" << evidence.type << "]</span> ";
                output << evidence.description;
                output << " <span class=\"evidence-confidence\">(confidence: " << std::fixed << std::setprecision(1)
                       << (evidence.confidence * 100) << "%)</span>\n";
                if (!evidence.source.empty()) {
                    output << "                    <div class=\"detail\">Source: " << evidence.source << "</div>\n";
                }
                output << "                </div>\n";
            }
            output << "            </div>\n";
        }
        output << "        </div>\n";
    }

    // Active scan results
    if (results.has_active_results()) {
        const auto& active = *results.active_results;
        output << "        <h2>Active Scan Results</h2>\n";
        output << "        <div class=\"stats\">\n";
        output << "            <div class=\"stat-box\"><div class=\"stat-label\">Tested</div>"
               << "<div class=\"stat-value\">" << active.candidates_tested_count << "</div></div>\n";
        output << "            <div class=\"stat-box\"><div class=\"stat-label\">Confirmed</div>"
               << "<div class=\"stat-value\">" << active.servers_confirmed_count << "</div></div>\n";
        output << "            <div class=\"stat-box\"><div class=\"stat-label\">Failed</div>"
               << "<div class=\"stat-value\">" << active.tests_failed_count << "</div></div>\n";
        output << "        </div>\n";

        output << "        <h2>Confirmed MCP Servers (" << active.confirmed_servers.size() << ")</h2>\n";
        for (size_t i = 0; i < active.confirmed_servers.size(); ++i) {
            const auto& s = active.confirmed_servers[i];
            output << "        <div class=\"server\">\n";
            output << "            <div class=\"server-title\">[" << (i + 1) << "] "
                   << s.server_name << " v" << s.server_version << "</div>\n";
            output << "            <div class=\"detail\">Protocol: " << s.protocol_version << "</div>\n";
            output << "            <div class=\"detail\">Endpoint: " << s.endpoint() << "</div>\n";

            // Tools
            if (!s.tools.empty()) {
                output << "            <div class=\"capability-section\">🔧 Tools (" << s.tools.size() << ")</div>\n";
                for (const auto& tool : s.tools) {
                    output << "            <div class=\"capability-item\">\n";
                    output << "                <div class=\"capability-name\">" << tool.name << "</div>\n";
                    if (!tool.description.empty()) {
                        output << "                <div class=\"capability-desc\">" << tool.description << "</div>\n";
                    }
                    if (!tool.required_parameters.empty() || !tool.optional_parameters.empty()) {
                        output << "                <div class=\"param-list\">\n";
                        for (const auto& param : tool.required_parameters) {
                            output << "                    <span class=\"param-badge param-required\">required: " << param << "</span>\n";
                        }
                        for (const auto& param : tool.optional_parameters) {
                            output << "                    <span class=\"param-badge param-optional\">optional: " << param << "</span>\n";
                        }
                        output << "                </div>\n";
                    }
                    output << "            </div>\n";
                }
            }

            // Resources
            if (!s.resources.empty()) {
                output << "            <div class=\"capability-section\">📁 Resources (" << s.resources.size() << ")</div>\n";
                for (const auto& resource : s.resources) {
                    output << "            <div class=\"capability-item\">\n";
                    output << "                <div class=\"capability-name\">" << resource.uri;
                    if (!resource.name.empty()) {
                        output << " (" << resource.name << ")";
                    }
                    output << "</div>\n";
                    if (!resource.description.empty()) {
                        output << "                <div class=\"capability-desc\">" << resource.description << "</div>\n";
                    }
                    if (!resource.mime_type.empty()) {
                        output << "                <div class=\"capability-desc\">Type: " << resource.mime_type << "</div>\n";
                    }
                    output << "            </div>\n";
                }
            }

            // Resource Templates
            if (!s.resource_templates.empty()) {
                output << "            <div class=\"capability-section\">📋 Resource Templates (" << s.resource_templates.size() << ")</div>\n";
                for (const auto& tmpl : s.resource_templates) {
                    output << "            <div class=\"capability-item\">\n";
                    output << "                <div class=\"capability-name\">" << tmpl.uri_template;
                    if (!tmpl.name.empty()) {
                        output << " (" << tmpl.name << ")";
                    }
                    output << "</div>\n";
                    if (!tmpl.description.empty()) {
                        output << "                <div class=\"capability-desc\">" << tmpl.description << "</div>\n";
                    }
                    if (!tmpl.parameters.empty()) {
                        output << "                <div class=\"param-list\">\n";
                        for (const auto& param : tmpl.parameters) {
                            output << "                    <span class=\"param-badge param-optional\">" << param << "</span>\n";
                        }
                        output << "                </div>\n";
                    }
                    output << "            </div>\n";
                }
            }

            // Prompts
            if (!s.prompts.empty()) {
                output << "            <div class=\"capability-section\">💬 Prompts (" << s.prompts.size() << ")</div>\n";
                for (const auto& prompt : s.prompts) {
                    output << "            <div class=\"capability-item\">\n";
                    output << "                <div class=\"capability-name\">" << prompt.name << "</div>\n";
                    if (!prompt.description.empty()) {
                        output << "                <div class=\"capability-desc\">" << prompt.description << "</div>\n";
                    }
                    if (!prompt.arguments.empty()) {
                        output << "                <div class=\"param-list\">\n";
                        for (const auto& arg : prompt.arguments) {
                            if (arg.required) {
                                output << "                    <span class=\"param-badge param-required\">required: " << arg.name << "</span>\n";
                            } else {
                                output << "                    <span class=\"param-badge param-optional\">optional: " << arg.name << "</span>\n";
                            }
                        }
                        output << "                </div>\n";
                    }
                    output << "            </div>\n";
                }
            }

            // Interrogation status
            if (s.interrogation_attempted && !s.interrogation_successful) {
                output << "            <div class=\"error\">Interrogation failed";
                if (!s.interrogation_errors.empty()) {
                    output << ": " << s.interrogation_errors[0];
                }
                output << "</div>\n";
            }

            output << "        </div>\n";
        }
    }

    // Errors
    if (!results.errors.empty()) {
        output << "        <h2>Errors</h2>\n";
        for (const auto& error : results.errors) {
            output << "        <div class=\"error\">" << error << "</div>\n";
        }
    }

    output << R"(    </div>
</body>
</html>
)";
}

} // namespace kyros
