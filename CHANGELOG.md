# Changelog

All notable changes to Kyros will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- Linux platform support
- Windows platform support
- SSE transport support
- Daemon mode with systemd/launchd integration
- Container detection (Docker, Kubernetes)
- Concurrent scanning
- Performance optimizations

## [2.0.0] - 2024-12-31

### Added
- Comprehensive Google Test framework integration
- Code coverage support with lcov/gcov
- GitHub Actions CI/CD pipeline
- Detailed testing documentation (TESTING.md)
- Enhanced architecture documentation
- Comprehensive usage guide
- Development guide for contributors
- Mock platform adapter for testing
- Test fixtures and helper utilities
- Code coverage reporting targets
- Sanitizer support (ASan, UBSan)
- Build script with multiple build types
- Enhanced error handling and logging
- Evidence-based confidence scoring system
- Complete server interrogation with limits
- Resource template support
- Prompt discovery and parameter extraction

### Changed
- Migrated from custom test framework to Google Test
- Improved CMake build system with FetchContent
- Enhanced platform abstraction layer
- Refactored scanner architecture for better testability
- Updated documentation structure and organization
- Improved error reporting across all components

### Fixed
- Statistics tracking across all detection engines
- CLI reporter display for config-detected servers
- Platform adapter NULL pointer issues
- Path expansion for paths with spaces
- Linker errors in macOS process implementation
- Memory leaks in testing engines
- Race conditions in process detection

## [1.0.0] - 2024-12-29

### Added
- Initial C++17 project structure
- CMake build system with cross-platform support
- Platform abstraction layer (macOS initial implementation)
- Multiple output formats (CLI, JSON, HTML, CSV)
- Detection engines (Config, Process, Network, Container stub)
- Testing engines (Stdio, HTTP)
- Server interrogation support (--interrogate flag)
- Statistics tracking for detection engines
- Comprehensive specification documentation
- Basic test framework
- CLI application with argument parsing
- JSON-RPC 2.0 protocol implementation
- MCP protocol version 2024-11-05 support

## Legend

- **Added** - New features
- **Changed** - Changes to existing functionality
- **Deprecated** - Soon-to-be removed features
- **Removed** - Removed features
- **Fixed** - Bug fixes
- **Security** - Security improvements
