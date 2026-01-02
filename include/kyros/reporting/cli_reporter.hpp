#ifndef KYROS_CLI_REPORTER_HPP
#define KYROS_CLI_REPORTER_HPP

#include <kyros/reporting/reporter.hpp>

namespace kyros {

class cliReporter : public Reporter {
public:
    std::string name() const override { return "cli"; }
    std::string file_extension() const override;
    void generate(const ScanResults& results, std::ostream& output) override;
};

} // namespace kyros

#endif
