#ifndef KYROS_PROCESS_HPP
#define KYROS_PROCESS_HPP

#include <chrono>
#include <string>

namespace kyros {

/**
 * Cross-platform process abstraction
 *
 * Represents a spawned process with stdin/stdout pipes
 */
class Process {
public:
    virtual ~Process() = default;

    // Process I/O
    virtual void write_stdin(const std::string& data) = 0;
    virtual std::string read_stdout_line(std::chrono::milliseconds timeout) = 0;
    virtual std::string read_stderr_line(std::chrono::milliseconds timeout) = 0;

    // Process control
    virtual void terminate() = 0;
    virtual bool is_running() const = 0;
    virtual int exit_code() const = 0;

    // Process information
    int pid() const { return pid_; }

protected:
    Process() = default;
    int pid_ = 0;
};

} // namespace kyros

#endif // KYROS_PROCESS_HPP
