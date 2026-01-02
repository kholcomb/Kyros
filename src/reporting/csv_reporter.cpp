#include <kyros/reporting/csv_reporter.hpp>
#include <iomanip>

namespace kyros {

std::string csvReporter::file_extension() const {
    return "csv";
}

static std::string escape_csv(const std::string& str) {
    if (str.find(',') == std::string::npos &&
        str.find('"') == std::string::npos &&
        str.find('\n') == std::string::npos) {
        return str;
    }

    std::string result = "\"";
    for (char c : str) {
        if (c == '"') {
            result += "\"\"";
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
}

void csvReporter::generate(const ScanResults& results, std::ostream& output) {
    // CSV format for candidates
    output << "Type,Name,PID,URL,Port,Confidence,Evidence Count\n";

    for (const auto& candidate : results.passive_results.candidates) {
        std::string type;
        std::string name;

        if (candidate.is_process_candidate()) {
            type = "Process";
            name = candidate.process_name;
        } else if (candidate.is_network_candidate()) {
            type = "Network";
            name = candidate.url;
        } else {
            type = "Unknown";
            name = "";
        }

        output << escape_csv(type) << ","
               << escape_csv(name) << ","
               << candidate.pid << ","
               << escape_csv(candidate.url) << ","
               << candidate.port << ","
               << std::fixed << std::setprecision(4) << candidate.confidence_score << ","
               << candidate.evidence.size() << "\n";
    }

    // If active results exist, output confirmed servers
    if (results.has_active_results()) {
        output << "\nConfirmed Servers\n";
        output << "Name,Version,Protocol,Endpoint,Tools,Resources,Prompts\n";

        for (const auto& server : results.active_results->confirmed_servers) {
            output << escape_csv(server.server_name) << ","
                   << escape_csv(server.server_version) << ","
                   << escape_csv(server.protocol_version) << ","
                   << escape_csv(server.endpoint()) << ","
                   << server.tools.size() << ","
                   << server.resources.size() << ","
                   << server.prompts.size() << "\n";
        }

        // Tools section
        bool has_tools = false;
        for (const auto& server : results.active_results->confirmed_servers) {
            if (!server.tools.empty()) {
                has_tools = true;
                break;
            }
        }

        if (has_tools) {
            output << "\nTools\n";
            output << "Server,Tool Name,Description,Required Parameters,Optional Parameters\n";

            for (const auto& server : results.active_results->confirmed_servers) {
                for (const auto& tool : server.tools) {
                    std::string required_params;
                    for (size_t i = 0; i < tool.required_parameters.size(); ++i) {
                        if (i > 0) required_params += "; ";
                        required_params += tool.required_parameters[i];
                    }

                    std::string optional_params;
                    for (size_t i = 0; i < tool.optional_parameters.size(); ++i) {
                        if (i > 0) optional_params += "; ";
                        optional_params += tool.optional_parameters[i];
                    }

                    output << escape_csv(server.server_name) << ","
                           << escape_csv(tool.name) << ","
                           << escape_csv(tool.description) << ","
                           << escape_csv(required_params) << ","
                           << escape_csv(optional_params) << "\n";
                }
            }
        }

        // Resources section
        bool has_resources = false;
        for (const auto& server : results.active_results->confirmed_servers) {
            if (!server.resources.empty()) {
                has_resources = true;
                break;
            }
        }

        if (has_resources) {
            output << "\nResources\n";
            output << "Server,URI,Name,Description,MIME Type\n";

            for (const auto& server : results.active_results->confirmed_servers) {
                for (const auto& resource : server.resources) {
                    output << escape_csv(server.server_name) << ","
                           << escape_csv(resource.uri) << ","
                           << escape_csv(resource.name) << ","
                           << escape_csv(resource.description) << ","
                           << escape_csv(resource.mime_type) << "\n";
                }
            }
        }

        // Resource Templates section
        bool has_templates = false;
        for (const auto& server : results.active_results->confirmed_servers) {
            if (!server.resource_templates.empty()) {
                has_templates = true;
                break;
            }
        }

        if (has_templates) {
            output << "\nResource Templates\n";
            output << "Server,URI Template,Name,Description,Parameters\n";

            for (const auto& server : results.active_results->confirmed_servers) {
                for (const auto& tmpl : server.resource_templates) {
                    std::string params;
                    for (size_t i = 0; i < tmpl.parameters.size(); ++i) {
                        if (i > 0) params += "; ";
                        params += tmpl.parameters[i];
                    }

                    output << escape_csv(server.server_name) << ","
                           << escape_csv(tmpl.uri_template) << ","
                           << escape_csv(tmpl.name) << ","
                           << escape_csv(tmpl.description) << ","
                           << escape_csv(params) << "\n";
                }
            }
        }

        // Prompts section
        bool has_prompts = false;
        for (const auto& server : results.active_results->confirmed_servers) {
            if (!server.prompts.empty()) {
                has_prompts = true;
                break;
            }
        }

        if (has_prompts) {
            output << "\nPrompts\n";
            output << "Server,Prompt Name,Description,Arguments\n";

            for (const auto& server : results.active_results->confirmed_servers) {
                for (const auto& prompt : server.prompts) {
                    std::string args;
                    for (size_t i = 0; i < prompt.arguments.size(); ++i) {
                        if (i > 0) args += "; ";
                        args += prompt.arguments[i].name;
                        if (prompt.arguments[i].required) {
                            args += " (required)";
                        } else {
                            args += " (optional)";
                        }
                    }

                    output << escape_csv(server.server_name) << ","
                           << escape_csv(prompt.name) << ","
                           << escape_csv(prompt.description) << ","
                           << escape_csv(args) << "\n";
                }
            }
        }
    }
}

} // namespace kyros
