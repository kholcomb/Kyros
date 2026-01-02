#ifndef KYROS_MACOS_PROCESS_HPP
#define KYROS_MACOS_PROCESS_HPP

#include <kyros/platform/process.hpp>

namespace kyros {

class MacOSProcess : public Process {
public:
    MacOSProcess(int pid, int stdin_fd, int stdout_fd, int stderr_fd);
    ~MacOSProcess() override;

    void write_stdin(const std::string& data) override;
    std::string read_stdout_line(std::chrono::milliseconds timeout) override;
    std::string read_stderr_line(std::chrono::milliseconds timeout) override;
    void terminate() override;
    bool is_running() const override;
    int exit_code() const override;

private:
    int stdin_fd_;
    int stdout_fd_;
    int stderr_fd_;
    int exit_code_;
    bool exited_;

    void close_fds();
    std::string read_line_from_fd(int fd, std::chrono::milliseconds timeout, const char* stream_name);
};

} // namespace kyros

#endif // KYROS_MACOS_PROCESS_HPP
