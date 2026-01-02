#ifndef KYROS_REPORTER_HPP
#define KYROS_REPORTER_HPP

#include <kyros/config.hpp>
#include <ostream>
#include <string>

namespace kyros {

class Reporter {
public:
    virtual ~Reporter() = default;

    virtual std::string name() const = 0;
    virtual std::string file_extension() const = 0;
    virtual void generate(const ScanResults& results, std::ostream& output) = 0;

    virtual void set_option([[maybe_unused]] const std::string& key,
                            [[maybe_unused]] const std::string& value) {}
};

} // namespace kyros

#endif
