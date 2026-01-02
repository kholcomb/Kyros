#include <kyros/platform/process.hpp>

namespace kyros {

// Process implementation is platform-specific and is provided by:
// - platform/linux/linux_process.cpp
// - platform/macos/macos_process.cpp
// - platform/windows/windows_process.cpp

// This file exists to satisfy the build system but contains no implementation
// as all methods are pure virtual and implemented in platform-specific files

} // namespace kyros
