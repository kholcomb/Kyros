#include <kyros/reporting/reporting_engine.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace kyros {

void ReportingEngine::register_reporter(std::shared_ptr<Reporter> reporter) {
    reporters_[reporter->name()] = reporter;
}

void ReportingEngine::generate_report(const std::string& reporter_name,
                                     const ScanResults& results,
                                     const std::string& output_file) {
    auto it = reporters_.find(reporter_name);
    if (it == reporters_.end()) {
        throw std::runtime_error("Reporter not found: " + reporter_name);
    }

    auto& reporter = it->second;

    if (output_file.empty()) {
        // Output to stdout
        reporter->generate(results, std::cout);
    } else {
        // Output to file
        std::ofstream file(output_file);
        if (!file) {
            throw std::runtime_error("Failed to open output file: " + output_file);
        }
        reporter->generate(results, file);
    }
}

} // namespace kyros
