#ifndef KYROS_CONTAINER_DETECTION_ENGINE_HPP
#define KYROS_CONTAINER_DETECTION_ENGINE_HPP

#include <kyros/detection/detection_engine.hpp>

namespace kyros {

class ContainerDetectionEngine : public DetectionEngine {
public:
    std::string name() const override;
    std::vector<Candidate> detect() override;

    // Statistics
    int get_last_scan_container_count() const {
        return last_scan_container_count_;
    }

private:
    // Helper methods for evidence detection
    void check_mcp_gateway(const DockerContainer&, Candidate&);
    void check_mcp_labels(const DockerContainer&, Candidate&);
    void check_mcp_entrypoint(const DockerContainer&, Candidate&);
    void check_mcp_environment(const DockerContainer&, Candidate&);

    // Statistics
    int last_scan_container_count_ = 0;
};

} // namespace kyros

#endif
