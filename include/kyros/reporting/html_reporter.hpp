#ifndef KYROS_HTML_REPORTER_HPP
#define KYROS_HTML_REPORTER_HPP

#include <kyros/reporting/reporter.hpp>

namespace kyros {

class htmlReporter : public Reporter {
public:
    std::string name() const override { return "html"; }
    std::string file_extension() const override;
    void generate(const ScanResults& results, std::ostream& output) override;
};

} // namespace kyros

#endif
