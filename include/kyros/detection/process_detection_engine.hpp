#ifndef KYROS_PROCESS_DETECTION_ENGINE_HPP
#define KYROS_PROCESS_DETECTION_ENGINE_HPP

#include <kyros/detection/detection_engine.hpp>

namespace kyros {

class ProcessDetectionEngine : public DetectionEngine {
public:
    std::string name() const override;
    std::vector<Candidate> detect() override;

    // Get statistics from last scan
    int get_last_scan_process_count() const { return last_scan_process_count_; }

private:
    // Helper methods for detection
    void check_parent_process(int pid, Candidate& candidate);
    void check_file_descriptors(int pid, Candidate& candidate);
    void check_environment(int pid, Candidate& candidate);

    // Statistics from last scan
    int last_scan_process_count_ = 0;
};

} // namespace kyros

#endif
