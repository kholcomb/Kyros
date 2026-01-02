#include <kyros/reporting/json_reporter.hpp>
#include <nlohmann/json.hpp>

namespace kyros {

std::string jsonReporter::file_extension() const {
    return "json";
}

void jsonReporter::generate(const ScanResults& results, std::ostream& output) {
    nlohmann::json report = nlohmann::json::object();

    // Passive results
    auto& passive_json = report["passive_scan"];
    passive_json["statistics"] = {
        {"config_files_checked", results.passive_results.config_files_checked},
        {"processes_scanned", results.passive_results.processes_scanned},
        {"network_sockets_checked", results.passive_results.network_sockets_checked},
        {"containers_scanned", results.passive_results.containers_scanned},
        {"scan_duration_seconds", results.passive_results.scan_duration_seconds}
    };

    auto& candidates_json = passive_json["candidates"] = nlohmann::json::array();
    for (const auto& candidate : results.passive_results.candidates) {
        nlohmann::json c = {
            {"pid", candidate.pid},
            {"process_name", candidate.process_name},
            {"command", candidate.command},
            {"url", candidate.url},
            {"port", candidate.port},
            {"confidence_score", candidate.confidence_score},
            {"evidence_count", candidate.evidence.size()}
        };

        // Always include evidence array in JSON (for machine parsing)
        auto& evidence_json = c["evidence"] = nlohmann::json::array();
        for (const auto& evidence : candidate.evidence) {
            nlohmann::json e = {
                {"type", evidence.type},
                {"description", evidence.description},
                {"confidence", evidence.confidence},
                {"source", evidence.source}
            };
            evidence_json.push_back(e);
        }

        candidates_json.push_back(c);
    }

    // Active results
    if (results.has_active_results()) {
        const auto& active = *results.active_results;
        auto& active_json = report["active_scan"];
        active_json["statistics"] = {
            {"candidates_tested", active.candidates_tested_count},
            {"servers_confirmed", active.servers_confirmed_count},
            {"tests_failed", active.tests_failed_count},
            {"scan_duration_seconds", active.scan_duration_seconds}
        };

        auto& servers_json = active_json["confirmed_servers"] = nlohmann::json::array();
        for (const auto& server : active.confirmed_servers) {
            nlohmann::json s = {
                {"server_name", server.server_name},
                {"server_version", server.server_version},
                {"protocol_version", server.protocol_version},
                {"endpoint", server.endpoint()},
                {"capabilities", server.capabilities}
            };

            // Tools
            auto& tools_json = s["tools"] = nlohmann::json::array();
            for (const auto& tool : server.tools) {
                nlohmann::json t = {
                    {"name", tool.name},
                    {"description", tool.description},
                    {"input_schema", tool.input_schema},
                    {"required_parameters", tool.required_parameters},
                    {"optional_parameters", tool.optional_parameters}
                };
                tools_json.push_back(t);
            }

            // Resources
            auto& resources_json = s["resources"] = nlohmann::json::array();
            for (const auto& resource : server.resources) {
                nlohmann::json r = {
                    {"uri", resource.uri},
                    {"name", resource.name},
                    {"description", resource.description},
                    {"mime_type", resource.mime_type}
                };
                resources_json.push_back(r);
            }

            // Resource Templates
            auto& templates_json = s["resource_templates"] = nlohmann::json::array();
            for (const auto& tmpl : server.resource_templates) {
                nlohmann::json rt = {
                    {"uri_template", tmpl.uri_template},
                    {"name", tmpl.name},
                    {"description", tmpl.description},
                    {"mime_type", tmpl.mime_type},
                    {"parameters", tmpl.parameters}
                };
                templates_json.push_back(rt);
            }

            // Prompts
            auto& prompts_json = s["prompts"] = nlohmann::json::array();
            for (const auto& prompt : server.prompts) {
                nlohmann::json p = {
                    {"name", prompt.name},
                    {"description", prompt.description}
                };

                auto& args_json = p["arguments"] = nlohmann::json::array();
                for (const auto& arg : prompt.arguments) {
                    nlohmann::json a = {
                        {"name", arg.name},
                        {"type", arg.type},
                        {"description", arg.description},
                        {"required", arg.required}
                    };
                    args_json.push_back(a);
                }

                prompts_json.push_back(p);
            }

            // Interrogation metadata
            s["interrogation"] = {
                {"attempted", server.interrogation_attempted},
                {"successful", server.interrogation_successful},
                {"errors", server.interrogation_errors},
                {"time_seconds", server.interrogation_time_seconds}
            };

            servers_json.push_back(s);
        }
    }

    // Errors
    report["errors"] = results.errors;

    output << report.dump(2) << "\n";
}

} // namespace kyros
