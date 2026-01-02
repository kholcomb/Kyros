# Kyros Rulepacks

Rulepacks allow you to customize and extend Kyros detection with custom rules that match specific patterns and adjust confidence scores.

## Overview

Rulepacks are JSON files that contain rules for:
- **Identifying known MCP servers** (add high-confidence evidence)
- **Boosting confidence** for specific patterns
- **Tagging candidates** for easier filtering
- **Setting minimum confidence levels**

## Default Rulepack

Kyros automatically loads the default rulepack (`default.json`) which includes:
- Official @modelcontextprotocol npm packages
- Common MCP servers (filesystem, github, postgres, etc.)
- Pattern-based confidence boosts

## Usage

### Load Custom Rulepack
```bash
kyros --rulepack /path/to/custom-rules.json
```

### Load Multiple Rulepacks
```bash
kyros --rulepack rules1.json --rulepack rules2.json
```

### With Verbose Output
```bash
kyros --verbose --rulepack custom.json
```

## Rulepack Format

```json
{
  "name": "My Custom Rulepack",
  "version": "1.0",
  "description": "Description of what these rules do",
  "rules": [
    {
      "name": "Rule Name",
      "description": "What this rule does",
      "match": {
        "match_type": "match_value"
      },
      "action": {
        "action_type": {
          "parameters": "values"
        }
      }
    }
  ]
}
```

## Match Types

All match conditions in a rule must be satisfied (AND logic).

### `process_name`
Match against process name (substring match).
```json
"match": {
  "process_name": "node"
}
```

### `command_contains`
Match if command line contains string.
```json
"match": {
  "command_contains": "@modelcontextprotocol"
}
```

### `command_regex`
Match command line against regex pattern.
```json
"match": {
  "command_regex": "npx\\s+@modelcontextprotocol/.*"
}
```

### `port`
Match exact port number.
```json
"match": {
  "port": 8080
}
```

### `url_contains`
Match if URL contains string.
```json
"match": {
  "url_contains": "localhost"
}
```

### `config_file`
Match if declared in specific config file.
```json
"match": {
  "config_file": "claude_desktop_config.json"
}
```

### `has_evidence_type`
Match if candidate already has evidence of specific type.
```json
"match": {
  "has_evidence_type": "parent_process"
}
```

## Action Types

### `add_evidence`
Add new evidence to the candidate (most flexible).
```json
"action": {
  "add_evidence": {
    "type": "known_mcp_server",
    "description": "Official MCP Filesystem server",
    "confidence": 0.9,
    "source": "rulepack:custom"
  }
}
```

**Note**: Evidence is combined using **Noisy-OR algorithm**:
- Multiple independent signals compound to higher confidence
- Duplicate evidence has diminishing returns
- Examples:
  - 1 signal at 70% → 70%
  - 2 signals at 70% → 91%
  - 2 signals at 30% → 51%

### `boost_confidence`
Multiply existing confidence by a factor.
```json
"action": {
  "boost_confidence": 1.5
}
```

**Warning**: This bypasses Noisy-OR and directly multiplies the final score. Use `add_evidence` instead for proper compounding.

### `set_minimum_confidence`
Ensure candidate has at least this confidence level.
```json
"action": {
  "set_minimum_confidence": 0.85
}
```

Useful for config-declared servers that should always have high confidence.

### `add_tag`
Add a descriptive tag (stored as zero-confidence evidence).
```json
"action": {
  "add_tag": "official-mcp-filesystem"
}
```

Tags don't affect confidence but can help with filtering and categorization.

## Complete Examples

### Example 1: Known MCP Server
```json
{
  "name": "MCP PostgreSQL Server",
  "description": "Official PostgreSQL MCP server",
  "match": {
    "command_contains": "@modelcontextprotocol/server-postgres"
  },
  "action": {
    "add_evidence": {
      "type": "known_mcp_server",
      "description": "MCP Postgres server",
      "confidence": 0.9,
      "source": "rulepack:custom"
    },
    "add_tag": "official-mcp-postgres"
  }
}
```

### Example 2: Pattern-Based Boost
```json
{
  "name": "Node.js MCP Pattern",
  "description": "Node.js processes with MCP parent likely run MCP servers",
  "match": {
    "process_name": "node",
    "has_evidence_type": "parent_process"
  },
  "action": {
    "add_evidence": {
      "type": "nodejs_mcp_pattern",
      "description": "Node.js process with MCP-aware parent",
      "confidence": 0.3,
      "source": "rulepack:custom"
    }
  }
}
```

### Example 3: Config-Declared Minimum
```json
{
  "name": "Config Declared Boost",
  "description": "Servers explicitly declared in config deserve high confidence",
  "match": {
    "has_evidence_type": "config_declared"
  },
  "action": {
    "set_minimum_confidence": 0.85
  }
}
```

### Example 4: Multiple Conditions
```json
{
  "name": "High-Confidence Development Server",
  "description": "Node.js on port 3000 with MCP parent is very likely MCP server",
  "match": {
    "process_name": "node",
    "port": 3000,
    "has_evidence_type": "parent_process"
  },
  "action": {
    "add_evidence": {
      "type": "high_confidence_pattern",
      "description": "Node.js on dev port with MCP parent",
      "confidence": 0.7,
      "source": "rulepack:custom"
    }
  }
}
```

## Best Practices

1. **Use `add_evidence` over `boost_confidence`**
   - Properly compounds with Noisy-OR algorithm
   - More transparent (visible in `--verbose` output)

2. **Set appropriate confidence levels**
   - 0.9: Known official servers
   - 0.7-0.8: Strong patterns (command line matches known package)
   - 0.3-0.5: Weak signals (common patterns like Node.js)
   - 0.1-0.2: Very weak hints

3. **Use meaningful evidence types**
   - `known_mcp_server`: Officially recognized server
   - `package_pattern`: Matches known package naming
   - `custom_pattern`: User-defined pattern
   - `tag`: Pure metadata (0 confidence)

4. **Add source attribution**
   - Use `"source": "rulepack:name"` format
   - Helps debug which rules are firing

5. **Test your rulepacks**
   ```bash
   kyros --verbose --rulepack your-rules.json
   ```

6. **Combine multiple conditions carefully**
   - All conditions must match (AND logic)
   - More conditions = more specific rule

## Default Rulepack Location

Kyros searches for default rulepacks in:
1. `./config/rulepacks/default.json` (current directory)
2. `../config/rulepacks/default.json` (parent directory)
3. `/usr/local/share/kyros/rulepacks/default.json` (system install)
4. `/usr/share/kyros/rulepacks/default.json` (system install)

## Confidence Calculation

Kyros uses the **Noisy-OR algorithm** to combine evidence:

```
confidence = 1 - ∏(1 - evidence[i].confidence)
```

This means:
- Independent signals compound correctly
- Duplicate evidence has diminishing returns
- Multiple strong signals can reach high confidence
- Maximum confidence is capped at 99%

**Example**:
```
Evidence 1: 70% (parent process is MCP client)
Evidence 2: 30% (network listener on common port)
Final: 1 - (0.3 × 0.7) = 79%
```

## Troubleshooting

### Rule not matching
- Use `--verbose` to see all evidence
- Check that ALL match conditions are satisfied
- Verify match values are exact (case-sensitive)

### Confidence not changing
- Rules apply BEFORE confidence filtering
- Check if candidate passes `min_confidence` threshold
- Verify evidence was added (check verbose output)

### Rulepack not loading
- Check JSON syntax (use a JSON validator)
- Verify file path exists
- Look for warnings in verbose output

## Contributing

To contribute rules to the default rulepack:
1. Test your rules thoroughly
2. Use conservative confidence values
3. Document what the rule detects
4. Submit a pull request with examples
