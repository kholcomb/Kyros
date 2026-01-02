# Kyros

**Model Context Protocol Server Detection Engine**

Kyros is a comprehensive detection and discovery engine for identifying Model Context Protocol (MCP) servers operating within computing environments. Written in modern C++17, Kyros provides both passive discovery and active confirmation capabilities to inventory MCP servers with detailed capability analysis.

## Features

- **Passive Detection** - Discover MCP servers without active interaction through configuration parsing, process inspection, and network enumeration
- **Active Confirmation** - Verify candidates through MCP protocol handshake and metadata extraction
- **Server Interrogation** - Extract detailed capabilities including tools, resources, resource templates, and prompts
- **Multiple Transports** - Support for stdio and HTTP transports with SSE planned
- **Cross-Platform** - Currently supports macOS with Linux and Windows support planned
- **Dual Deployment** - Standalone CLI tool or daemon service mode
- **Multiple Output Formats** - Human-readable CLI, machine-parsable JSON, web-viewable HTML, and CSV exports
- **Evidence-Based Scoring** - Confidence scoring based on multiple detection signals
- **Comprehensive Testing** - Full test coverage with Google Test framework and continuous integration

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                       Kyros ENGINE                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │  PASSIVE    │───▶│   ACTIVE     │───▶│ CORRELATION  │  │
│  │   SCAN      │    │   PROBE      │    │   ENGINE     │  │
│  └─────────────┘    └──────────────┘    └──────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Building

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, Apple Clang 10+, MSVC 2017+)
- CMake 3.15 or later
- Git for version control

**Optional Dependencies:**
- SQLite3 (for daemon mode)
- lcov/gcov (for code coverage)

**Note:** Third-party dependencies (nlohmann/json, CLI11, Google Test) are automatically fetched via CMake FetchContent.

### Quick Build

```bash
# Clone repository
git clone https://github.com/your-org/kyros.git
cd kyros

# Build using provided script
./build.sh

# Binary will be available at build/kyros
./build/kyros --help
```

### Manual Build

```bash
# Create build directory
mkdir build && cd build

# Configure with desired options
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON

# Build (use -j for parallel compilation)
cmake --build . -j$(nproc)

# Run tests
ctest --output-on-failure

# Install (optional)
sudo cmake --install .
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_DAEMON` | ON | Build daemon service mode |
| `BUILD_TESTS` | ON | Build tests |
| `BUILD_EXAMPLES` | ON | Build examples |
| `ENABLE_CONTAINERS` | ON | Enable Docker/Kubernetes support |
| `ENABLE_MANAGEMENT_SERVER` | ON | Enable management server client |

## Usage

### CLI Mode

```bash
# Passive discovery (find potential MCP servers)
./build/kyros

# Active confirmation (verify with protocol handshake)
./build/kyros --mode active

# Full interrogation (extract detailed capabilities)
./build/kyros --mode active --interrogate

# JSON output for automation
./build/kyros --mode active --format json -o scan-results.json

# HTML report with detailed information
./build/kyros --mode active --interrogate --format html -o report.html

# Adjust timeout for slow-starting servers
./build/kyros --mode active --timeout 10000

# Verbose output for debugging
./build/kyros --mode active --interrogate --verbose
```

### Daemon Mode (Planned)

Daemon mode with systemd/launchd integration is planned for future releases. Currently, Kyros operates as a standalone CLI tool.

## Documentation

Comprehensive documentation is available in the `docs/` directory:

- **[Usage Guide](docs/Usage.md)** - Command-line interface reference, scan modes, and output formats
- **[Architecture](docs/Architecture.md)** - System architecture, components, and data structures
- **[Development Guide](docs/Development.md)** - Build system, project structure, and contribution guidelines
- **[Testing Guide](docs/TESTING.md)** - Test framework, coverage reporting, and CI/CD integration
- **[Complete Specification](spec/Kyros.md)** - Detailed technical specification and design rationale

## Project Structure

```
kyros/
├── include/kyros/      # Public headers
│   ├── detection/         # Detection engines
│   ├── testing/           # Testing engines
│   ├── reporting/         # Reporters
│   ├── platform/          # Platform abstraction
│   ├── scan_types/        # Scan type configs
│   └── daemon/            # Daemon components
├── src/                   # Implementation
│   ├── detection/
│   ├── testing/
│   ├── reporting/
│   ├── platform/
│   │   ├── linux/
│   │   ├── macos/
│   │   └── windows/
│   └── daemon/
├── tests/                 # Unit and integration tests
├── examples/              # Example usage
├── third_party/           # External dependencies
├── packaging/             # Distribution packages
│   ├── linux/
│   ├── macos/
│   └── windows/
└── docs/                  # Additional documentation
```

## Testing

Kyros includes comprehensive test coverage using Google Test:

```bash
# Run all tests
cd build && ctest --output-on-failure

# Generate code coverage report
./build.sh Coverage --coverage
open build/coverage/html/index.html
```

For detailed testing information, see [TESTING.md](docs/TESTING.md).

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](./CONTRIBUTING.md) for guidelines on:

- Development setup and workflow
- Code style and conventions
- Pull request process
- Testing requirements

## License

Kyros is released under the MIT License. See [LICENSE](./LICENSE) file for details.

Copyright (c) 2024 Kyros Contributors

## Support

- **Issues:** Report bugs and request features through GitHub Issues
- **Documentation:** See the `docs/` directory for comprehensive guides
- **Specification:** Review `spec/Kyros.md` for technical details

## Acknowledgments

Kyros builds upon several excellent open-source libraries:

- [nlohmann/json](https://github.com/nlohmann/json) - JSON parsing
- [CLI11](https://github.com/CLIUtils/CLI11) - Command-line parsing
- [Google Test](https://github.com/google/googletest) - Testing framework
