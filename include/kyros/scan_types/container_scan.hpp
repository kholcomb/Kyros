#ifndef KYROS_CONTAINER_SCAN_HPP
#define KYROS_CONTAINER_SCAN_HPP

#include <kyros/scan_types/scan_type.hpp>

namespace kyros {

class ContainerScan : public ScanType {
public:
    std::string name() const override { return "Container Scan"; }
    bool is_available() const override;  // Checks for Docker/K8s

    void set_scan_docker(bool scan) { scan_docker_ = scan; }
    void set_scan_kubernetes(bool scan) { scan_k8s_ = scan; }

private:
    bool scan_docker_ = true;
    bool scan_k8s_ = true;
};

} // namespace kyros

#endif
