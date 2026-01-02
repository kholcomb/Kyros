#ifndef KYROS_DETECTION_ENGINE_HPP
#define KYROS_DETECTION_ENGINE_HPP

#include <kyros/candidate.hpp>
#include <kyros/evidence.hpp>
#include <kyros/platform/platform_adapter.hpp>

#include <memory>
#include <string>
#include <vector>

namespace kyros {

/**
 * Base class for detection engines
 *
 * Detection engines discover MCP server candidates through
 * passive observation (no interaction with processes/network)
 */
class DetectionEngine {
public:
    virtual ~DetectionEngine() = default;

    // Pure virtual - must be implemented
    virtual std::string name() const = 0;
    virtual std::vector<Candidate> detect() = 0;

    // Virtual with default - can be overridden
    virtual bool requires_elevated_privileges() const { return false; }

    // Concrete - shared by all derived classes
    void set_platform_adapter(std::shared_ptr<PlatformAdapter> adapter) {
        platform_ = adapter;
    }

protected:
    DetectionEngine() = default;
    std::shared_ptr<PlatformAdapter> platform_;

    // Helper to create evidence
    Evidence make_evidence(const std::string& type,
                          const std::string& description,
                          double confidence,
                          const std::string& source = "") const {
        return Evidence(type, description, confidence, source);
    }
};

} // namespace kyros

#endif // KYROS_DETECTION_ENGINE_HPP
