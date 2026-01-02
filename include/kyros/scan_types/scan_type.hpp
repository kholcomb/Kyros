#ifndef KYROS_SCAN_TYPE_HPP
#define KYROS_SCAN_TYPE_HPP

#include <string>

namespace kyros {

/**
 * Base class for scan type configuration
 */
class ScanType {
public:
    virtual ~ScanType() = default;

    // Pure virtual - must be implemented by derived classes
    virtual std::string name() const = 0;
    virtual bool is_available() const = 0;

    // Concrete - shared by all derived classes
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }

protected:
    ScanType() = default;
    bool enabled_ = true;
};

} // namespace kyros

#endif // KYROS_SCAN_TYPE_HPP
