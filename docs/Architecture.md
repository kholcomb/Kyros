# Kyros Architecture

## Overview

Kyros is a Model Context Protocol (MCP) server discovery and interrogation tool. It employs a three-phase scanning approach: passive detection, active confirmation, and optional capability interrogation.

## System Architecture

```mermaid
graph TD
    Scanner[Scanner]
    PassiveScanner[PassiveScanner]
    ActiveScanner[ActiveScanner]
    DetectionEngines[Detection Engines]
    TestingEngines[Testing Engines]
    PlatformAdapter[Platform Adapter<br/>macOS, Linux, Windows]

    Scanner --> PassiveScanner
    Scanner --> ActiveScanner
    PassiveScanner --> DetectionEngines
    ActiveScanner --> TestingEngines
    DetectionEngines --> PlatformAdapter
    TestingEngines --> PlatformAdapter

    style Scanner fill:#e1f5ff
    style PassiveScanner fill:#fff4e1
    style ActiveScanner fill:#fff4e1
    style DetectionEngines fill:#e8f5e9
    style TestingEngines fill:#e8f5e9
    style PlatformAdapter fill:#f3e5f5
```

## Core Components

### Scanner

The main orchestrator that coordinates passive detection and active testing phases.

**Responsibilities:**
- Execute scan workflow based on configuration
- Coordinate detection and testing engines
- Aggregate and deduplicate results
- Delegate reporting to ReportingEngine

**Key Methods:**
- `scan(ScanConfig)` - Main entry point for all scan operations

### PassiveScanner

Discovers potential MCP servers without active probing.

**Detection Engines:**
- `ConfigDetectionEngine` - Parses configuration files (Claude Desktop, custom configs)
- `ProcessDetectionEngine` - Identifies running MCP processes via parent process inspection and stdio pipe detection
- `NetworkDetectionEngine` - Discovers HTTP listeners on localhost
- `ContainerDetectionEngine` - Stub for future Docker/Kubernetes support

**Output:** List of `Candidate` objects with evidence-based confidence scores.

### ActiveScanner

Confirms candidates by performing MCP protocol handshakes.

**Testing Engines:**
- `StdioTestingEngine` - Tests stdio-based MCP servers via process pipes
- `HttpTestingEngine` - Tests HTTP-based MCP servers via POST requests

**Optional Interrogation:**
- `ServerInterrogator` - Extracts detailed server capabilities when enabled

**Output:** List of confirmed `MCPServer` objects with protocol metadata.

### ServerInterrogator

Extracts detailed capability information from confirmed MCP servers.

**Capabilities Extracted:**
- Tools (via `tools/list` request)
- Resources (via `resources/list` request)
- Resource Templates (via `resources/templates/list` request)
- Prompts (via `prompts/list` request)

**Features:**
- Configurable limits to prevent response overflow
- Timeout support for unresponsive servers
- Works with both stdio and HTTP transports
- Comprehensive error tracking

## Platform Abstraction Layer

Provides OS-specific implementations for system operations.

### PlatformAdapter Interface

**File Operations:**
- `file_exists()` - Check file existence
- `expand_path()` - Handle ~ and environment variables
- `read_json_file()` - Parse JSON configuration files

**Process Operations:**
- `spawn_process_with_pipes()` - Create child process with stdio redirection
- `get_process_list()` - Enumerate running processes
- `get_command_line()` - Retrieve process command line
- `get_parent_pid()` - Find parent process
- `has_bidirectional_pipes()` - Detect stdio-based IPC

**Network Operations:**
- `get_listening_sockets()` - Find processes listening on network ports

### MacOSPlatformAdapter

Current implementation using macOS system APIs:
- `proc_listallpids()` for process enumeration
- `proc_pidpath()` for executable path retrieval
- `proc_pidinfo()` for process metadata and file descriptor inspection
- `lsof` for network socket detection
- Unix fork and execute primitives for process spawning

## Data Structures

```mermaid
classDiagram
    class Candidate {
        +string command
        +string url
        +int pid
        +TransportType transport_hint
        +vector~Evidence~ evidence
        +double confidence
        +add_evidence(Evidence)
    }

    class Evidence {
        +string type
        +string description
        +double confidence
    }

    class MCPServer {
        +Candidate candidate
        +string server_name
        +string server_version
        +string protocol_version
        +json capabilities
        +TransportType transport_type
        +vector~ToolDefinition~ tools
        +vector~ResourceDefinition~ resources
        +vector~ResourceTemplate~ resource_templates
        +vector~PromptDefinition~ prompts
        +bool interrogation_attempted
        +bool interrogation_successful
        +has_tools() bool
        +has_resources() bool
        +has_prompts() bool
    }

    class ToolDefinition {
        +string name
        +string description
        +json input_schema
        +vector~string~ required_parameters
        +vector~string~ optional_parameters
    }

    class ResourceDefinition {
        +string uri
        +string name
        +string description
        +string mime_type
    }

    class ResourceTemplate {
        +string uri_template
        +string name
        +string description
        +string mime_type
        +vector~string~ parameters
    }

    class PromptDefinition {
        +string name
        +string description
        +vector~PromptArgument~ arguments
    }

    class PromptArgument {
        +string name
        +string type
        +string description
        +bool required
    }

    Candidate "1" *-- "many" Evidence
    MCPServer "1" *-- "1" Candidate
    MCPServer "1" *-- "many" ToolDefinition
    MCPServer "1" *-- "many" ResourceDefinition
    MCPServer "1" *-- "many" ResourceTemplate
    MCPServer "1" *-- "many" PromptDefinition
    PromptDefinition "1" *-- "many" PromptArgument
```

### Candidate

Represents a potential MCP server discovered during passive scanning.

**Fields:**
- `command` - Command line to execute (stdio servers)
- `url` - HTTP endpoint (HTTP servers)
- `pid` - Process ID (running servers)
- `transport_hint` - Expected transport type
- `evidence` - Collection of detection signals
- `confidence` - Calculated from evidence strength

### MCPServer

Represents a confirmed MCP server with protocol metadata.

**Fields:**
- `server_name` - Server identification
- `server_version` - Server version string
- `protocol_version` - MCP protocol version
- `capabilities` - Server-declared capabilities
- `transport_type` - Confirmed transport (Stdio/HTTP)
- `tools` - List of available tools (if interrogated)
- `resources` - List of available resources (if interrogated)
- `resource_templates` - List of resource templates (if interrogated)
- `prompts` - List of available prompts (if interrogated)

### Evidence

Individual piece of detection information.

**Fields:**
- `type` - Evidence category (config_declared, parent_process, etc.)
- `description` - Human-readable explanation
- `confidence` - Strength of signal (0.0 - 1.0)

## Scan Workflow

### Passive Scan

```mermaid
flowchart TD
    Start([Start Passive Scan]) --> ConfigScan[Configuration Scan]
    ConfigScan --> ParseClaudeConfig[Parse Claude Desktop Config]
    ConfigScan --> ParseCustomConfig[Parse Custom Configs]
    ParseClaudeConfig --> CreateConfigCandidates[Create Candidates]
    ParseCustomConfig --> CreateConfigCandidates

    Start --> ProcessScan[Process Scan]
    ProcessScan --> EnumProcesses[Enumerate Running Processes]
    EnumProcesses --> CheckParent[Check Parent Process]
    CheckParent --> InspectFDs[Inspect File Descriptors]
    InspectFDs --> CreateProcessCandidates[Create Candidates]

    Start --> NetworkScan[Network Scan]
    NetworkScan --> EnumSockets[Enumerate Listening Sockets]
    EnumSockets --> FilterLocalhost[Filter Localhost Only]
    FilterLocalhost --> CreateNetworkCandidates[Create HTTP Candidates]

    CreateConfigCandidates --> Dedup[Deduplication]
    CreateProcessCandidates --> Dedup
    CreateNetworkCandidates --> Dedup
    Dedup --> MergeEvidence[Merge Evidence & Calculate Confidence]
    MergeEvidence --> Results([Candidate List with Confidence Scores])

    style Start fill:#e1f5ff
    style Results fill:#c8e6c9
    style Dedup fill:#fff9c4
    style MergeEvidence fill:#fff9c4
```

**Steps:**
1. **Configuration Scan** - Parse config files and create candidates from declared servers
2. **Process Scan** - Enumerate processes, check relationships, inspect file descriptors
3. **Network Scan** - Find listening sockets and create HTTP URL candidates
4. **Deduplication** - Merge candidates by unique identifiers and combine evidence

### Active Scan

```mermaid
flowchart TD
    Start([Start Active Scan]) --> ForEach{For Each Candidate}
    ForEach --> CheckTransport{Transport Type?}

    CheckTransport -->|Stdio/Command| StdioEngine[StdioTestingEngine]
    CheckTransport -->|HTTP/URL| HttpEngine[HttpTestingEngine]

    StdioEngine --> SpawnProcess[Spawn Process with Pipes]
    HttpEngine --> SendHTTP[Send HTTP POST]

    SpawnProcess --> SendInitialize[Send MCP Initialize Request]
    SendHTTP --> SendInitialize

    SendInitialize --> ReadResponse[Read Response]
    ReadResponse --> ValidateRPC{Valid JSON-RPC 2.0?}

    ValidateRPC -->|No| MarkFailed[Mark as Failed]
    ValidateRPC -->|Yes| ExtractMetadata[Extract Server Metadata]

    ExtractMetadata --> CheckInterrogate{Interrogation Enabled?}

    CheckInterrogate -->|No| CreateServer[Create MCPServer Object]
    CheckInterrogate -->|Yes| Interrogate[ServerInterrogator]

    Interrogate --> QueryTools[Query tools/list]
    Interrogate --> QueryResources[Query resources/list]
    Interrogate --> QueryTemplates[Query resources/templates/list]
    Interrogate --> QueryPrompts[Query prompts/list]

    QueryTools --> ParseResponses[Parse Responses]
    QueryResources --> ParseResponses
    QueryTemplates --> ParseResponses
    QueryPrompts --> ParseResponses

    ParseResponses --> CreateServer
    CreateServer --> AddToConfirmed[Add to Confirmed Servers]
    MarkFailed --> AddToFailed[Add to Failed Tests]

    AddToConfirmed --> ForEach
    AddToFailed --> ForEach
    ForEach -->|Done| Results([Active Scan Results])

    style Start fill:#e1f5ff
    style Results fill:#c8e6c9
    style Interrogate fill:#fff9c4
    style CheckInterrogate fill:#ffe0b2
    style ValidateRPC fill:#ffe0b2
```

**Steps:**
1. **Candidate Testing** - Try applicable testing engines based on transport type
2. **Protocol Handshake** - Send MCP initialize request and validate JSON-RPC 2.0 response
3. **Optional Interrogation** - If enabled, query server for tools, resources, templates, and prompts

### Reporting

1. **Format Selection**
   - CLI (human-readable terminal output)
   - JSON (machine-parsable)
   - HTML (web-viewable)
   - CSV (spreadsheet-compatible)

2. **Content**
   - Scan statistics and timing
   - Discovered candidates with confidence scores
   - Confirmed servers with protocol details
   - Interrogation results (if enabled)
   - Error log

## Configuration

### ScanConfig

**Scan Mode:**
- `PassiveOnly` - Discovery without confirmation
- `ActiveOnly` - Test provided candidates
- `PassiveThenActive` - Full scan workflow

**Passive Configuration:**
- `scan_configs` - Enable config file detection
- `scan_processes` - Enable process detection
- `scan_network` - Enable network detection
- `min_confidence` - Filter candidates by confidence threshold

**Active Configuration:**
- `probe_timeout_ms` - Maximum time per server test
- `interrogate` - Enable capability interrogation
- `interrogation_config` - Detailed interrogation settings

**Interrogation Configuration:**
- `get_tools` - Extract tool definitions
- `get_resources` - Extract resource definitions
- `get_resource_templates` - Extract resource templates
- `get_prompts` - Extract prompt definitions
- `max_tools/max_resources/max_prompts` - Limit response sizes
- `timeout` - Interrogation request timeout

## Error Handling

### Strategy

Kyros employs graceful degradation:
- Engine failures are logged but do not halt scanning
- Individual candidate test failures are tracked
- Interrogation errors are recorded per-server

### Error Collection

All errors are aggregated in scan results:
- `PassiveScanResults.errors` - Detection engine failures
- `ActiveScanResults.errors` - Testing and interrogation failures
- `ScanResults.errors` - Consolidated error list

## Thread Safety

Current implementation is single-threaded. Concurrent scanning is planned for future versions.

## Component Interaction

```mermaid
sequenceDiagram
    participant User
    participant Scanner
    participant PassiveScanner
    participant DetectionEngine
    participant ActiveScanner
    participant TestingEngine
    participant ServerInterrogator
    participant PlatformAdapter

    User->>Scanner: scan(config)

    alt Passive Scan Enabled
        Scanner->>PassiveScanner: scan(passive_config)
        PassiveScanner->>DetectionEngine: detect()
        DetectionEngine->>PlatformAdapter: get_process_list()
        PlatformAdapter-->>DetectionEngine: process list
        DetectionEngine->>PlatformAdapter: get_listening_sockets()
        PlatformAdapter-->>DetectionEngine: socket list
        DetectionEngine-->>PassiveScanner: candidates with evidence
        PassiveScanner-->>Scanner: PassiveScanResults
    end

    alt Active Scan Enabled
        Scanner->>ActiveScanner: scan(candidates, active_config)
        loop For each candidate
            ActiveScanner->>TestingEngine: test(candidate)
            TestingEngine->>PlatformAdapter: spawn_process_with_pipes()
            PlatformAdapter-->>TestingEngine: Process handle
            TestingEngine->>TestingEngine: Send MCP initialize
            TestingEngine->>TestingEngine: Validate response
            TestingEngine-->>ActiveScanner: MCPServer or nullopt

            alt Interrogation Enabled
                ActiveScanner->>ServerInterrogator: interrogate(server)
                ServerInterrogator->>ServerInterrogator: Query tools/list
                ServerInterrogator->>ServerInterrogator: Query resources/list
                ServerInterrogator->>ServerInterrogator: Query prompts/list
                ServerInterrogator->>ServerInterrogator: Parse responses
                ServerInterrogator-->>ActiveScanner: Updated MCPServer
            end
        end
        ActiveScanner-->>Scanner: ActiveScanResults
    end

    Scanner-->>User: ScanResults (complete)
```

## Extension Points

### Adding Detection Engines

1. Implement `DetectionEngine` interface
2. Add engine to `PassiveScanner::initialize_engines()`
3. Engine should generate `Candidate` objects with appropriate evidence

### Adding Testing Engines

1. Implement `TestingEngine` interface
2. Add engine to `ActiveScanner::initialize_engines()`
3. Engine should return `std::optional<MCPServer>` on success

### Adding Platform Support

1. Implement `PlatformAdapter` interface for target OS
2. Create platform-specific `Process` implementation
3. Update factory to instantiate appropriate adapter

### Adding Report Formats

1. Implement `Reporter` interface
2. Add reporter to `ReportingEngine::initialize_reporters()`
3. Update CLI format validation
