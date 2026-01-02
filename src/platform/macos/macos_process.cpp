// macOS Process Implementation

#include <kyros/platform/macos/macos_process.hpp>
#include <stdexcept>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>
#include <cstring>

namespace kyros {

MacOSProcess::MacOSProcess(int pid, int stdin_fd, int stdout_fd, int stderr_fd)
    : stdin_fd_(stdin_fd)
    , stdout_fd_(stdout_fd)
    , stderr_fd_(stderr_fd)
    , exit_code_(-1)
    , exited_(false)
{
    pid_ = pid;
}

MacOSProcess::~MacOSProcess() {
    if (is_running()) {
        terminate();
    }
    close_fds();
}

void MacOSProcess::write_stdin(const std::string& data) {
    if (stdin_fd_ < 0) {
        throw std::runtime_error("stdin pipe not available");
    }

    size_t total_written = 0;
    while (total_written < data.size()) {
        ssize_t written = write(stdin_fd_, data.c_str() + total_written,
                               data.size() - total_written);

        if (written < 0) {
            if (errno == EPIPE) {
                throw std::runtime_error("Broken pipe - process may have terminated");
            } else if (errno != EINTR) {
                throw std::runtime_error(std::string("Failed to write to stdin: ") +
                                       strerror(errno));
            }
        } else {
            total_written += written;
        }
    }
}

std::string MacOSProcess::read_stdout_line(std::chrono::milliseconds timeout) {
    return read_line_from_fd(stdout_fd_, timeout, "stdout");
}

std::string MacOSProcess::read_stderr_line(std::chrono::milliseconds timeout) {
    return read_line_from_fd(stderr_fd_, timeout, "stderr");
}

void MacOSProcess::terminate() {
    if (!is_running()) {
        return;
    }

    // Send SIGTERM for graceful shutdown
    if (kill(pid_, SIGTERM) == 0) {
        // Wait briefly for graceful termination
        for (int i = 0; i < 10; ++i) {
            if (!is_running()) {
                close_fds();
                return;
            }
            usleep(100000); // 100ms
        }
    }

    // Force kill if still running
    if (is_running()) {
        kill(pid_, SIGKILL);
        waitpid(pid_, &exit_code_, 0);
        exited_ = true;
    }

    close_fds();
}

bool MacOSProcess::is_running() const {
    if (exited_) {
        return false;
    }

    int status;
    pid_t result = waitpid(pid_, &status, WNOHANG);

    if (result == 0) {
        // Process is still running
        return true;
    } else if (result == pid_) {
        // Process has exited
        const_cast<MacOSProcess*>(this)->exit_code_ = WIFEXITED(status) ?
            WEXITSTATUS(status) : -1;
        const_cast<MacOSProcess*>(this)->exited_ = true;
        return false;
    } else {
        // Error or no such process
        return false;
    }
}

int MacOSProcess::exit_code() const {
    if (!exited_) {
        throw std::runtime_error("Process has not exited yet");
    }
    return exit_code_;
}

void MacOSProcess::close_fds() {
    if (stdin_fd_ >= 0) {
        close(stdin_fd_);
        stdin_fd_ = -1;
    }
    if (stdout_fd_ >= 0) {
        close(stdout_fd_);
        stdout_fd_ = -1;
    }
    if (stderr_fd_ >= 0) {
        close(stderr_fd_);
        stderr_fd_ = -1;
    }
}

std::string MacOSProcess::read_line_from_fd(int fd, std::chrono::milliseconds timeout,
                              const char* stream_name) {
    if (fd < 0) {
        throw std::runtime_error(std::string(stream_name) + " pipe not available");
    }

    std::string line;
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (true) {
        // Calculate remaining timeout
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            throw std::runtime_error(std::string("Timeout reading from ") + stream_name);
        }

        auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(
            deadline - now);

        // Use select for timeout
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);

        struct timeval tv;
        tv.tv_sec = remaining.count() / 1000000;
        tv.tv_usec = remaining.count() % 1000000;

        int result = select(fd + 1, &read_fds, nullptr, nullptr, &tv);

        if (result < 0) {
            if (errno == EINTR) {
                continue; // Interrupted, retry
            }
            throw std::runtime_error(std::string("select() failed on ") + stream_name +
                                   ": " + strerror(errno));
        } else if (result == 0) {
            throw std::runtime_error(std::string("Timeout reading from ") + stream_name);
        }

        // Data available, read one character
        char ch;
        ssize_t bytes_read = read(fd, &ch, 1);

        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue; // Interrupted, retry
            }
            throw std::runtime_error(std::string("Failed to read from ") + stream_name +
                                   ": " + strerror(errno));
        } else if (bytes_read == 0) {
            // EOF - return what we have
            if (!line.empty()) {
                return line;
            }
            throw std::runtime_error(std::string("EOF on ") + stream_name);
        }

        if (ch == '\n') {
            return line; // Complete line
        }

        line += ch;
    }
}

} // namespace kyros
