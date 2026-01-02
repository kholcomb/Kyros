#ifndef KYROS_PROCESS_SCAN_HPP
#define KYROS_PROCESS_SCAN_HPP

#include <kyros/scan_types/scan_type.hpp>

namespace kyros {

class ProcessScan : public ScanType {
public:
    std::string name() const override { return "Process Table Scan"; }
    bool is_available() const override { return true; }

    void set_check_parent_process(bool check) { check_parent_ = check; }
    void set_check_file_descriptors(bool check) { check_fds_ = check; }

private:
    bool check_parent_ = true;
    bool check_fds_ = true;
};

} // namespace kyros

#endif
