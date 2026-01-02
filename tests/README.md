# Kyros Test Suite

This directory contains the comprehensive test suite for Kyros.

## Quick Reference

### Run All Tests
```bash
cd build && ctest --output-on-failure
```

### Run Specific Test
```bash
./build/test_interrogator
./build/test_detection_engines
./build/test_platform_adapters
```

### With Coverage
```bash
./build.sh Coverage --coverage
open build/coverage/html/index.html
```

## Directory Structure

- **unit/** - Unit tests for individual components
- **integration/** - End-to-end integration tests
- **fixtures/** - Test data and fixtures
- **mocks/** - Mock implementations for testing
- **utils/** - Shared test utilities and helpers

## Available Test Suites

### Unit Tests

1. **test_interrogator** (test_scanner.cpp)
   - ServerInterrogator functionality
   - JSON-RPC request/response handling
   - Tools, resources, prompts parsing
   - Limit enforcement

2. **test_detection_engines**
   - Process detection engine
   - Network detection engine
   - Config detection engine
   - Container detection engine

3. **test_platform_adapters**
   - Platform abstraction layer
   - Process information retrieval
   - Network connection tracking
   - Mock platform adapter

4. **test_rulepack**
   - Rulepack loading and parsing
   - Rule validation
   - JSON serialization

5. **test_evidence**
   - Evidence creation and scoring
   - Evidence aggregation
   - Confidence calculations

### Integration Tests

1. **integration_tests**
   - End-to-end scan workflows
   - Passive to active pipeline
   - Report generation

## Test Utilities

### test_helpers.hpp

Provides common helper functions:

- `create_test_server()` - Create sample MCP servers
- `create_test_tool()` - Create sample tools
- `create_test_resource()` - Create sample resources
- `create_test_evidence()` - Create sample evidence
- `create_jsonrpc_request()` - Create JSON-RPC requests
- `create_jsonrpc_response()` - Create JSON-RPC responses
- `TempFile` - Temporary file helper class

### Mock Objects

- **mock_platform_adapter.hpp** - Mock platform adapter for controlled testing

## Adding New Tests

1. Create test file in `unit/` or `integration/`
2. Include necessary headers:
   ```cpp
   #include <gtest/gtest.h>
   #include "test_helpers.hpp"
   ```
3. Write tests using Google Test macros
4. Add to `CMakeLists.txt`:
   ```cmake
   add_kyros_test(test_name
       SOURCES unit/test_name.cpp
   )
   ```

## Writing Tests

### Basic Test Structure

```cpp
TEST(TestSuiteName, TestName) {
    // Arrange
    auto object = create_test_object();

    // Act
    auto result = object.method();

    // Assert
    EXPECT_EQ(result, expected);
}
```

### Using Test Fixtures

```cpp
class MyTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code
    }

    void TearDown() override {
        // Cleanup code
    }
};

TEST_F(MyTestFixture, TestName) {
    // Test code
}
```

## Google Test Features Used

- **TEST()** - Basic test cases
- **TEST_F()** - Tests with fixtures
- **TEST_P()** - Parameterized tests
- **EXPECT_EQ/NE/LT/GT/LE/GE** - Comparison assertions
- **EXPECT_TRUE/FALSE** - Boolean assertions
- **EXPECT_THAT()** - Matcher assertions
- **ASSERT_*()** - Fatal assertions

## GMock Features Used

- **MOCK_METHOD()** - Define mock methods
- **EXPECT_CALL()** - Set expectations
- **WillOnce/WillRepeatedly** - Define return behavior
- **Return()** - Return values from mocks

## Coverage Goals

- Overall: 80%+
- Critical components: 90%+
- All public APIs: 100%

## Documentation

See [docs/TESTING.md](../docs/TESTING.md) for comprehensive testing guide.
