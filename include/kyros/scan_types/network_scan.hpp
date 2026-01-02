#ifndef KYROS_NETWORK_SCAN_HPP
#define KYROS_NETWORK_SCAN_HPP

#include <kyros/scan_types/scan_type.hpp>

namespace kyros {

class NetworkScan : public ScanType {
public:
    std::string name() const override { return "Network Socket Scan"; }
    bool is_available() const override { return true; }

    void set_scan_localhost_only(bool localhost_only) { localhost_only_ = localhost_only; }
    void set_port_range(int start, int end) { port_start_ = start; port_end_ = end; }

private:
    bool localhost_only_ = false;
    int port_start_ = 1;
    int port_end_ = 65535;
};

} // namespace kyros

#endif
