#ifndef KYROS_JSON_REPORTER_HPP
#define KYROS_JSON_REPORTER_HPP

#include <kyros/reporting/reporter.hpp>

namespace kyros {

class jsonReporter : public Reporter {
public:
    std::string name() const override { return "json"; }
    std::string file_extension() const override;
    void generate(const ScanResults& results, std::ostream& output) override;
};

} // namespace kyros

#endif
