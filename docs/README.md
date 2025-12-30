# Kyros Documentation

Kyros is a Model Context Protocol (MCP) server discovery and interrogation tool. It provides comprehensive scanning capabilities for detecting, confirming, and analyzing MCP servers in local environments.

## Documentation Overview

### [Architecture](Architecture.md)

Technical architecture and system design documentation.

**Contents:**
- System architecture overview
- Core component descriptions
- Data structure specifications
- Scan workflow details
- Configuration reference
- Extension points

**Audience:** Developers, system architects

### [Usage](Usage.md)

End-user guide for running Kyros scans.

**Contents:**
- Installation instructions
- Command line interface reference
- Scan mode descriptions
- Output format examples
- Troubleshooting guide
- Advanced usage patterns

**Audience:** End users, system administrators

### [Development](Development.md)

Guide for developers contributing to Kyros.

**Contents:**
- Build system documentation
- Project structure overview
- Testing guidelines
- Feature addition procedures
- Code style conventions
- Platform-specific notes

**Audience:** Contributors, maintainers

## Quick Start

### Build and Run

```bash
# Build
./build.sh

# Discover MCP servers
./build/kyros

# Confirm and interrogate
./build/kyros --mode active --interrogate
```

### Core Concepts

**Passive Scanning**
- Configuration file parsing
- Running process detection
- Network listener enumeration
- Evidence-based confidence scoring

**Active Testing**
- MCP protocol handshake
- Server confirmation
- Metadata extraction
- Transport validation

**Interrogation**
- Tools discovery
- Resources enumeration
- Prompts listing
- Capability analysis

## Key Features

### Multi-Phase Scanning

1. **Detection** - Discover potential MCP servers
2. **Confirmation** - Verify via protocol handshake
3. **Interrogation** - Extract detailed capabilities

### Evidence-Based Scoring

Multiple detection signals combine to produce confidence scores:
- Configuration declarations (0.9)
- Parent process relationships (0.7)
- Stdio pipe detection (0.6)
- Environment variables (0.5)
- Network listeners (0.3-0.5)

### Platform Abstraction

Portable design with platform-specific implementations:
- macOS (current)
- Linux (planned)
- Windows (planned)

### Multiple Transport Support

- Stdio (process pipes)
- HTTP (network endpoints)
- SSE (planned)

### Flexible Reporting

- CLI (human-readable)
- JSON (machine-parsable)
- HTML (web-viewable)
- CSV (spreadsheet-compatible)

## MCP Protocol Support

Kyros implements the Model Context Protocol specification:

**Protocol Version:** 2024-11-05

**Supported Operations:**
- `initialize` - Server handshake and capability negotiation
- `tools/list` - Tool discovery
- `resources/list` - Resource enumeration
- `resources/templates/list` - Template discovery
- `prompts/list` - Prompt listing

## Implementation Status

### Phase 1: Core Engine - Complete

- Configuration detection
- Stdio testing
- HTTP testing
- Basic reporting

### Phase 2: Passive Detection - Complete

- Process detection
- Network detection
- Evidence aggregation
- Confidence scoring

### Phase 3: Interrogation - Complete

- Tools extraction
- Resources extraction
- Resource templates extraction
- Prompts extraction
- Parameter parsing
- Limit enforcement

### Planned Features

- Container detection (Docker, Kubernetes)
- Enhanced reporters with interrogation display
- Concurrent scanning
- Additional platform support
- SSE transport support
- Daemon mode

## Technical Specifications

### Language

C++17

### Dependencies

- CMake 3.15+
- nlohmann/json (FetchContent)
- CLI11 (FetchContent)
- Platform system libraries

### Binary Size

Approximately 715KB (Release build)

### Performance

- Single-threaded execution
- Configurable timeouts
- Graceful degradation on errors

## Additional Resources

### External Documentation

- [MCP Specification](https://spec.modelcontextprotocol.io/)
- [Claude Desktop](https://claude.ai/desktop)

### Project Resources

- Build system: CMake
- Version control: Git
- Issue tracking: GitHub Issues
- Development plan: spec/planv2.md

## Getting Help

### Common Questions

**Q: No servers found?**
A: Ensure MCP servers are running and Claude Desktop is active.

**Q: Permission errors?**
A: Some detection features require elevated privileges on certain platforms.

**Q: Timeout errors?**
A: Increase timeout with `--timeout` flag or check server responsiveness.

### Reporting Issues

When reporting issues, include:
- Operating system and version
- Kyros version (`kyros --version`)
- Command line used
- Full error output
- Expected vs actual behavior

## License

See LICENSE file in repository root.
