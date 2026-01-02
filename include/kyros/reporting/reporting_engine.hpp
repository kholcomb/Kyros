#ifndef KYROS_REPORTING_ENGINE_HPP
#define KYROS_REPORTING_ENGINE_HPP

#include <kyros/reporting/reporter.hpp>
#include <map>
#include <memory>

namespace kyros {

class ReportingEngine {
public:
    void register_reporter(std::shared_ptr<Reporter> reporter);
    void generate_report(const std::string& reporter_name, 
                        const ScanResults& results,
                        const std::string& output_file = "");

private:
    std::map<std::string, std::shared_ptr<Reporter>> reporters_;
};

} // namespace kyros

#endif
