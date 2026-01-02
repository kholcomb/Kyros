#ifndef KYROS_NETWORK_DETECTION_ENGINE_HPP
#define KYROS_NETWORK_DETECTION_ENGINE_HPP

#include <kyros/detection/detection_engine.hpp>

namespace kyros {

class NetworkDetectionEngine : public DetectionEngine {
public:
    std::string name() const override;
    std::vector<Candidate> detect() override;

    // Get statistics from last scan
    int get_last_scan_socket_count() const { return last_scan_socket_count_; }

private:
    // Statistics from last scan
    int last_scan_socket_count_ = 0;
};

} // namespace kyros

#endif
