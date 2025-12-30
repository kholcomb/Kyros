# Kyros Development Guide

## Build System

### Requirements

- CMake 3.15 or later
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Standard POSIX environment (macOS, Linux)

### Building

```bash
# Full build with tests
./build.sh

# Build only (no tests)
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# Build with debugging symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `BUILD_DAEMON` | Build daemon components | OFF |
| `BUILD_TESTS` | Build test suite | ON |
| `BUILD_EXAMPLES` | Build example programs | OFF |
| `ENABLE_CONTAINERS` | Enable container detection | ON |

Example:
```bash
cmake -DBUILD_DAEMON=ON -DBUILD_TESTS=OFF ..
```

## Project Structure

```
kyros/
├── include/kyros/          # Public headers
│   ├── scanner.hpp         # Main scanner interface
│   ├── config.hpp          # Configuration structures
│   ├── types.hpp           # Common type definitions
│   ├── detection/          # Detection engine interfaces
│   ├── testing/            # Testing engine interfaces
│   ├── platform/           # Platform abstraction
│   └── reporting/          # Reporter interfaces
├── src/                    # Implementation files
│   ├── scanner.cpp         # Scanner orchestration
│   ├── detection/          # Detection engines
│   ├── testing/            # Testing engines
│   ├── platform/           # Platform implementations
│   └── reporting/          # Report generators
├── tests/                  # Test suite
│   └── unit/               # Unit tests
├── third_party/            # External dependencies
└── docs/                   # Documentation
```

## Testing

### Running Tests

```bash
# Run all tests
cd build && ctest

# Run with verbose output
ctest -V

# Run specific test
./tests/test_scanner
```

### Writing Tests

Tests use a minimal test framework with macros:

```cpp
void test_example() {
    TEST("Example test case")
        kyros::Config config;
        ASSERT(config.is_valid(), "Config should be valid");
    PASS()
}
```

## Adding Features

### New Detection Engine

1. Create header in `include/kyros/detection/`:
```cpp
class MyDetectionEngine : public DetectionEngine {
public:
    std::string name() const override { return "MyEngine"; }
    std::vector<Candidate> detect() override;
};
```

2. Implement in `src/detection/`:
```cpp
std::vector<Candidate> MyDetectionEngine::detect() {
    std::vector<Candidate> candidates;
    // Detection logic
    return candidates;
}
```

3. Register in `PassiveScanner::initialize_engines()`:
```cpp
auto engine = std::make_unique<MyDetectionEngine>();
engine->set_platform_adapter(platform_);
engines_.push_back(std::move(engine));
```

### New Testing Engine

1. Create header in `include/kyros/testing/`:
```cpp
class MyTestingEngine : public TestingEngine {
public:
    std::string name() const override { return "MyTester"; }
    std::optional<MCPServer> test(const Candidate& candidate) override;
};
```

2. Implement test logic:
```cpp
std::optional<MCPServer> MyTestingEngine::test(const Candidate& candidate) {
    // Protocol handshake
    // Return MCPServer on success, nullopt on failure
}
```

3. Register in `ActiveScanner::initialize_engines()`.

### New Platform Support

1. Implement `PlatformAdapter` interface for target OS
2. Implement `Process` interface for process management
3. Update `create_platform_adapter()` factory function
4. Add platform-specific system call implementations

## Code Style

### Naming Conventions

- **Classes:** PascalCase (`ServerInterrogator`)
- **Functions:** snake_case (`detect_servers()`)
- **Variables:** snake_case (`server_list`)
- **Constants:** UPPER_SNAKE_CASE (`MAX_SERVERS`)
- **Private members:** trailing underscore (`platform_`)

### File Organization

- One class per file
- Header guards: `KYROS_COMPONENT_NAME_HPP`
- Forward declarations in headers
- Implementation details in `.cpp` files

### Error Handling

- Use exceptions for exceptional conditions
- Return `std::optional` for expected failures
- Log errors to results structures
- Never silently fail

Example:
```cpp
std::optional<MCPServer> test(const Candidate& candidate) {
    try {
        // Test logic
        return server;
    } catch (const std::exception& e) {
        // Log error, return nullopt
        return std::nullopt;
    }
}
```

## Dependencies

### Required

- **nlohmann/json** - JSON parsing (via FetchContent)
- **CLI11** - Command line parsing (via FetchContent)

### Platform-Specific

- **macOS:** libproc (system)
- **Linux:** procfs (planned)
- **Windows:** Windows API (planned)

### Adding Dependencies

Use CMake FetchContent for header-only libraries:

```cmake
FetchContent_Declare(
    library_name
    URL https://github.com/user/library/archive/version.tar.gz
    URL_HASH SHA256=...
)
FetchContent_MakeAvailable(library_name)
```

## Debugging

### Verbose Mode

Enable detailed logging:
```bash
kyros --mode active --verbose
```

### Debug Build

Build with debugging symbols:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
lldb ./build/kyros
```

### Common Issues

**Process detection not working:**
- Check platform adapter implementation
- Verify system API availability
- Test with elevated privileges

**Interrogation timing out:**
- Increase timeout configuration
- Check server response time
- Verify transport connectivity

**Tests failing:**
- Rebuild completely: `rm -rf build && ./build.sh`
- Check for dependency version conflicts
- Verify platform compatibility

## Contributing

### Before Submitting

1. Run full test suite: `ctest`
2. Verify build: `./build.sh`
3. Check code style consistency
4. Update documentation as needed
5. Add tests for new functionality

### Pull Request Guidelines

- Clear description of changes
- Reference related issues
- Include test coverage
- Update relevant documentation
- Follow existing code patterns

## Performance Optimization

### Profiling

Use standard profiling tools:

```bash
# macOS
instruments -t "Time Profiler" ./build/kyros --mode active

# Linux
valgrind --tool=callgrind ./build/kyros --mode active
```

### Optimization Targets

- Process enumeration (currently O(n))
- JSON parsing (consider streaming for large responses)
- Network scanning (parallel socket checks)
- Candidate deduplication (consider hash-based approach)

## Platform Notes

### macOS

- Uses `proc_listallpids()` for process enumeration
- Requires `libproc` for process introspection
- Environment variable access limited without root
- File descriptor inspection via `proc_pidinfo()`

### Linux (Planned)

- Will use `/proc` filesystem
- Standard POSIX process APIs
- Better environment variable access
- Standard file descriptor inspection

### Windows (Planned)

- Will use Windows API
- Process enumeration via `CreateToolhelp32Snapshot`
- Different pipe mechanisms
- Registry configuration detection
