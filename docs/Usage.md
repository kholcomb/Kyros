# Kyros Usage Guide

## Installation

### Building from Source

```bash
./build.sh
```

Binary will be created at `build/kyros`.

### System Requirements

- macOS 10.15 or later (current implementation)
- C++17 compatible compiler
- CMake 3.15 or later

## Command Line Interface

### Basic Usage

```bash
# Passive scan only (discovery without confirmation)
kyros

# Active scan (discover and confirm)
kyros --mode active

# Full scan with interrogation
kyros --mode active --interrogate
```

### Options

| Option | Description | Default |
|--------|-------------|---------|
| `--mode <mode>` | Scan mode: passive, active | passive |
| `--interrogate` | Extract server capabilities | false |
| `--format <fmt>` | Output format: cli, json, html, csv | cli |
| `--output <file>` | Write output to file | stdout |
| `--timeout <ms>` | Probe timeout in milliseconds | 5000 |
| `--verbose` | Enable verbose logging | false |
| `--version` | Display version information | - |
| `--help` | Show help message | - |

## Scan Modes

### Passive Mode

Discovers potential MCP servers without active testing.

**Detection Methods:**
- Configuration file parsing
- Running process inspection
- Network listener enumeration

**Output:**
- List of candidate servers
- Confidence scores based on evidence
- No server confirmation

**Use Case:** Quick discovery with minimal system impact.

```bash
kyros --mode passive --format json -o candidates.json
```

### Active Mode

Confirms candidates by performing MCP protocol handshakes.

**Process:**
1. Discovers candidates (if not provided)
2. Tests each candidate via appropriate transport
3. Confirms servers that respond correctly to MCP initialize

**Output:**
- Confirmed MCP servers only
- Protocol version and capabilities
- Server metadata

**Use Case:** Verification of MCP servers with protocol validation.

```bash
kyros --mode active --timeout 10000
```

### Active with Interrogation

Extracts detailed capability information from confirmed servers.

**Additional Data Extracted:**
- Available tools with input schemas
- Available resources with metadata
- Resource templates with parameters
- Available prompts with arguments

**Output:**
- All active mode information
- Detailed capability listings
- Tool parameter specifications
- Resource template patterns

**Use Case:** Complete MCP server inventory with capability analysis.

```bash
kyros --mode active --interrogate --format html -o report.html
```

## Output Formats

### CLI (Default)

Human-readable terminal output with formatted tables and summaries.

```bash
kyros --mode active
```

### JSON

Machine-parsable structured data suitable for automation.

```bash
kyros --mode active --format json -o scan-results.json
```

**Structure:**
```json
{
  "scan_timestamp": "2024-12-29T12:00:00Z",
  "scan_duration_seconds": 2.5,
  "passive_results": { ... },
  "active_results": { ... },
  "errors": []
}
```

### HTML

Web-viewable report with styled formatting.

```bash
kyros --mode active --interrogate --format html -o report.html
```

### CSV

Spreadsheet-compatible tabular data.

```bash
kyros --mode active --format csv -o servers.csv
```

## Examples

### Find All MCP Servers

```bash
kyros --mode active --format json -o mcp-inventory.json
```

### Analyze Claude Desktop Configuration

```bash
# Passive scan to see configured servers
kyros

# Confirm they are running
kyros --mode active
```

### Extract Server Capabilities

```bash
kyros --mode active --interrogate --format html -o capabilities.html
```

### Quick Network Scan

```bash
# Find HTTP MCP servers on localhost
kyros --mode active --timeout 2000
```

### Verbose Debugging

```bash
kyros --mode active --interrogate --verbose
```

## Configuration Files

### Supported Formats

Kyros automatically detects MCP servers from:

**Claude Desktop Configuration:**
- macOS: `~/Library/Application Support/Claude/claude_desktop_config.json`
- Linux: `~/.config/Claude/claude_desktop_config.json`

**Custom Configuration:**
Additional paths can be specified via passive scan configuration.

### Configuration File Format

```json
{
  "mcpServers": {
    "filesystem": {
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-filesystem", "/path/to/allowed/files"],
      "env": {
        "CUSTOM_VAR": "value"
      }
    },
    "github": {
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-github"],
      "env": {
        "GITHUB_PERSONAL_ACCESS_TOKEN": "token"
      }
    },
    "http-server": {
      "url": "http://localhost:3000"
    }
  }
}
```

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success, servers found |
| 1 | Success, no servers found |
| 2 | Error during execution |

## Performance Considerations

### Timeout Values

- Default timeout: 5000ms
- Minimum recommended: 1000ms
- Maximum allowed: 60000ms

Lower timeouts reduce scan time but may miss slow-starting servers.

### Interrogation Limits

Interrogation has built-in limits to prevent overwhelming responses:
- Max tools: 100
- Max resources: 100
- Max prompts: 50

These can be adjusted via configuration.

## Troubleshooting

### No Servers Found

**Possible Causes:**
- No MCP servers running
- Servers using non-standard configurations
- Insufficient permissions for process inspection

**Solutions:**
- Verify MCP servers are running
- Check Claude Desktop is active
- Run with elevated privileges if needed

### Timeout Errors

**Possible Causes:**
- Servers slow to initialize
- Network latency (HTTP servers)
- Resource constraints

**Solutions:**
- Increase timeout: `--timeout 10000`
- Check server logs for issues
- Verify server is responsive

### Permission Errors

**Possible Causes:**
- Restricted process inspection (macOS)
- File system permissions

**Solutions:**
- Some platform features require elevated privileges
- Process environment variables may not be accessible without root

## Advanced Usage

### Programmatic Usage

Kyros can be integrated into automation workflows:

```bash
# Generate JSON report for processing
kyros --mode active --interrogate --format json | jq '.active_results.confirmed_servers'

# Check if specific server is running
kyros --mode active --format json | jq -e '.active_results.servers_confirmed_count > 0'
```

### Filtering Results

Use `jq` or similar tools to filter JSON output:

```bash
# Find servers with specific tools
kyros --mode active --interrogate --format json | \
  jq '.active_results.confirmed_servers[] | select(.tools[]?.name == "read_file")'

# List all available tools across all servers
kyros --mode active --interrogate --format json | \
  jq '.active_results.confirmed_servers[].tools[].name' | sort -u
```
