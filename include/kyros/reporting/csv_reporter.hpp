#ifndef KYROS_CSV_REPORTER_HPP
#define KYROS_CSV_REPORTER_HPP

#include <kyros/reporting/reporter.hpp>

namespace kyros {

class csvReporter : public Reporter {
public:
    std::string name() const override { return "csv"; }
    std::string file_extension() const override;
    void generate(const ScanResults& results, std::ostream& output) override;
};

} // namespace kyros

#endif
