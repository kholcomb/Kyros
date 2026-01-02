#include <kyros/scan_types/container_scan.hpp>

#include <cstdlib>

namespace kyros {

bool ContainerScan::is_available() const {
    #ifdef ENABLE_CONTAINERS
    // Check if Docker is available
    bool docker_available = (std::system("docker info > /dev/null 2>&1") == 0);

    // Check if kubectl is available (indicates Kubernetes)
    bool k8s_available = (std::system("kubectl version --client > /dev/null 2>&1") == 0);

    return (scan_docker_ && docker_available) || (scan_k8s_ && k8s_available);
    #else
    return false;
    #endif
}

} // namespace kyros
