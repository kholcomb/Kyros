/**
 * Kyros: Model Context Protocol Server Detection Engine
 * Main entry point
 */

#include <kyros/kyros.hpp>
#include <kyros/version.hpp>

#include <CLI/CLI.hpp>
#include <iostream>
#include <cstdlib>

// CLI argument parsing with CLI11
struct CliArgs {
    std::string mode = "passive";
    std::string format = "cli";
    std::string output_file;
    bool interrogate = false;
    bool verbose = false;
    int timeout = 5000;
    bool show_version = false;
    bool show_help = false;
    std::vector<std::string> rulepack_paths;

#ifdef ENABLE_DAEMON
    bool daemon_mode = false;
    std::string daemon_command;  // start, stop, restart, status
    std::string config_file;
#endif
};

void print_version() {
    std::cout << "Kyros version " << kyros::version_string << "\n";
    std::cout << "Model Context Protocol Server Detection Engine\n";
}

// CLI11 generates help automatically, but we can add custom footer
std::string get_examples_text() {
    return R"(
EXAMPLES:
    # Quick passive discovery
    kyros

    # Active confirmation
    kyros --mode active

    # Full discovery with interrogation
    kyros --mode active --interrogate

    # JSON output to file
    kyros --mode active --format json -o scan.json

    # Start daemon service (if enabled)
    kyros daemon start
)";
}

int run_scan(const CliArgs& args) {
    try {
        // Create scanner
        kyros::Scanner scanner;

        // Load custom rulepacks if specified
        for (const auto& rulepack_path : args.rulepack_paths) {
            if (args.verbose) {
                std::cout << "Loading custom rulepack: " << rulepack_path << "\n";
            }
            scanner.load_rulepack(rulepack_path);
        }

        // Configure scan
        kyros::ScanConfig config;

        // Set mode
        if (args.mode == "active") {
            config.mode = kyros::ScanMode::PassiveThenActive;
        } else {
            config.mode = kyros::ScanMode::PassiveOnly;
        }

        // Set active options
        config.active_config.interrogate = args.interrogate;
        config.active_config.probe_timeout_ms = args.timeout;

        // Set output options
        config.verbose = args.verbose;
        config.output_format = args.format;
        config.output_file = args.output_file;

        if (args.verbose) {
            std::cout << "Starting Kyros scan...\n";
            std::cout << "Mode: " << args.mode << "\n";
            std::cout << "Format: " << args.format << "\n";
        }

        // Run scan
        auto results = scanner.scan(config);

        // Generate report
        scanner.reporting_engine().generate_report(
            args.format,
            results,
            args.output_file
        );

        // Return appropriate exit code
        if (config.mode == kyros::ScanMode::PassiveOnly) {
            // Passive mode: exit 0 if candidates found
            return results.candidates().empty() ? 1 : 0;
        } else {
            // Active mode: exit 0 if servers confirmed
            return results.confirmed_servers().empty() ? 1 : 0;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}

#ifdef ENABLE_DAEMON
#include <kyros/daemon/daemon.hpp>

int run_daemon(const CliArgs& args) {
    // Daemon functionality will be implemented here
    std::cerr << "Daemon mode not yet implemented\n";
    return 1;
}
#endif

int main(int argc, char** argv) {
    CLI::App app{"Kyros - Model Context Protocol Server Detection Engine"};
    app.footer(get_examples_text());

    CliArgs args;

    // Version flag
    bool show_version = false;
    app.add_flag("--version", show_version, "Show version information");

    // Scan mode
    app.add_option("-m,--mode", args.mode, "Scan mode: passive, active")
        ->check(CLI::IsMember({"passive", "active"}))
        ->default_val("passive");

    // Output format
    app.add_option("-f,--format", args.format, "Output format: cli, json, html, csv")
        ->check(CLI::IsMember({"cli", "json", "html", "csv"}))
        ->default_val("cli");

    // Output file
    app.add_option("-o,--output", args.output_file, "Write output to file");

    // Interrogate flag
    app.add_flag("--interrogate", args.interrogate, "Interrogate confirmed servers");

    // Timeout
    app.add_option("-t,--timeout", args.timeout, "Probe timeout in milliseconds")
        ->default_val(5000)
        ->check(CLI::Range(100, 60000));

    // Verbose flag
    app.add_flag("-v,--verbose", args.verbose, "Increase output verbosity");

    // Rulepack paths
    app.add_option("-r,--rulepack", args.rulepack_paths, "Load custom rulepack file(s)")
        ->check(CLI::ExistingFile);

#ifdef ENABLE_DAEMON
    // Daemon subcommand
    auto* daemon_cmd = app.add_subcommand("daemon", "Daemon service management");
    daemon_cmd->add_option("command", args.daemon_command, "Daemon command (start, stop, restart, status)")
        ->required()
        ->check(CLI::IsMember({"start", "stop", "restart", "status", "reload", "scan-now"}));
    daemon_cmd->callback([&args]() { args.daemon_mode = true; });
#endif

    // Parse arguments
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    // Handle version flag
    if (show_version) {
        print_version();
        return 0;
    }

#ifdef ENABLE_DAEMON
    if (args.daemon_mode) {
        return run_daemon(args);
    }
#endif

    return run_scan(args);
}
