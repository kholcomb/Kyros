#include <kyros/http/http_client.hpp>

#include <sstream>
#include <stdexcept>
#include <cstdio>
#include <memory>
#include <array>

namespace kyros {

HttpClient::ParsedUrl HttpClient::parse_url(const std::string& url) {
    ParsedUrl result;

    // Find scheme
    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        return result;  // Invalid URL
    }

    result.scheme = url.substr(0, scheme_end);
    size_t authority_start = scheme_end + 3;

    // Find path start
    size_t path_start = url.find('/', authority_start);
    std::string authority;

    if (path_start == std::string::npos) {
        authority = url.substr(authority_start);
        result.path = "/";
    } else {
        authority = url.substr(authority_start, path_start - authority_start);
        result.path = url.substr(path_start);
    }

    // Parse host and port
    // Handle IPv6 addresses in brackets [::1] or [2001:db8::1]
    if (authority[0] == '[') {
        // IPv6 address - find closing bracket
        size_t bracket_end = authority.find(']');
        if (bracket_end == std::string::npos) {
            return result;  // Invalid IPv6 format
        }

        result.host = authority.substr(0, bracket_end + 1);  // Include brackets

        // Check for port after bracket
        if (bracket_end + 1 < authority.length() && authority[bracket_end + 1] == ':') {
            try {
                result.port = std::stoi(authority.substr(bracket_end + 2));
            } catch (...) {
                return result;  // Invalid port
            }
        } else {
            result.port = (result.scheme == "https") ? 443 : 80;
        }
    } else {
        // IPv4 or hostname
        size_t port_sep = authority.find(':');
        if (port_sep != std::string::npos) {
            result.host = authority.substr(0, port_sep);
            try {
                result.port = std::stoi(authority.substr(port_sep + 1));
            } catch (...) {
                return result;  // Invalid port
            }
        } else {
            result.host = authority;
            result.port = (result.scheme == "https") ? 443 : 80;
        }
    }

    result.valid = !result.host.empty();
    return result;
}

HttpResponse HttpClient::post(const std::string& url,
                              const std::string& body,
                              const std::map<std::string, std::string>& headers,
                              std::chrono::milliseconds timeout) {
    HttpResponse response;

    // Parse URL
    auto parsed = parse_url(url);
    if (!parsed.valid) {
        response.error_message = "Invalid URL";
        return response;
    }

    // Build curl command
    std::ostringstream cmd;
    cmd << "curl -X POST";
    cmd << " -m " << (timeout.count() / 1000);  // Timeout in seconds
    cmd << " -s";  // Silent mode
    cmd << " -w '\\n%{http_code}'";  // Write status code at end
    cmd << " -H 'Content-Type: application/json'";

    // Add custom headers
    for (const auto& [key, value] : headers) {
        cmd << " -H '" << key << ": " << value << "'";
    }

    // Add body
    cmd << " -d '" << body << "'";

    // Add URL
    cmd << " '" << url << "'";

    // Execute curl command
    std::array<char, 128> buffer;
    std::string result;
    FILE* pipe = popen(cmd.str().c_str(), "r");

    if (!pipe) {
        response.error_message = "Failed to execute curl command";
        return response;
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    int exit_code = pclose(pipe);

    if (exit_code != 0) {
        response.error_message = "curl command failed";
        return response;
    }

    // Parse response - last line should be status code
    size_t last_newline = result.rfind('\n');
    if (last_newline != std::string::npos && last_newline > 0) {
        size_t second_last_newline = result.rfind('\n', last_newline - 1);
        if (second_last_newline != std::string::npos) {
            std::string status_str = result.substr(second_last_newline + 1, last_newline - second_last_newline - 1);
            response.body = result.substr(0, second_last_newline);

            try {
                response.status_code = std::stoi(status_str);
                response.success = (response.status_code >= 200 && response.status_code < 300);
            } catch (...) {
                response.error_message = "Failed to parse status code";
            }
        }
    }

    return response;
}

HttpResponse HttpClient::get(const std::string& url,
                             const std::map<std::string, std::string>& headers,
                             std::chrono::milliseconds timeout) {
    HttpResponse response;

    // Parse URL
    auto parsed = parse_url(url);
    if (!parsed.valid) {
        response.error_message = "Invalid URL";
        return response;
    }

    // Build curl command with header dump
    std::ostringstream cmd;
    cmd << "curl -X GET";
    cmd << " -m " << (timeout.count() / 1000);  // Timeout in seconds
    cmd << " -s";  // Silent mode
    cmd << " -i";  // Include headers in output
    cmd << " -w '\\n%{http_code}'";  // Write status code at end

    // Add custom headers
    for (const auto& [key, value] : headers) {
        cmd << " -H '" << key << ": " << value << "'";
    }

    // Add URL
    cmd << " '" << url << "'";

    // Execute curl command
    std::array<char, 128> buffer;
    std::string result;
    FILE* pipe = popen(cmd.str().c_str(), "r");

    if (!pipe) {
        response.error_message = "Failed to execute curl command";
        return response;
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    int exit_code = pclose(pipe);

    if (exit_code != 0) {
        response.error_message = "curl command failed";
        return response;
    }

    // Parse response - last line should be status code
    size_t last_newline = result.rfind('\n');
    if (last_newline == std::string::npos) {
        response.error_message = "Invalid response format";
        return response;
    }

    size_t second_last_newline = result.rfind('\n', last_newline - 1);
    if (second_last_newline == std::string::npos) {
        response.error_message = "Invalid response format";
        return response;
    }

    std::string status_str = result.substr(second_last_newline + 1, last_newline - second_last_newline - 1);
    std::string full_response = result.substr(0, second_last_newline);

    try {
        response.status_code = std::stoi(status_str);
        response.success = (response.status_code >= 200 && response.status_code < 300);
    } catch (...) {
        response.error_message = "Failed to parse status code";
        return response;
    }

    // Parse headers and body (separated by \r\n\r\n or \n\n)
    size_t header_end = full_response.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        header_end = full_response.find("\n\n");
    }

    if (header_end != std::string::npos) {
        std::string header_section = full_response.substr(0, header_end);
        response.body = full_response.substr(header_end + (full_response[header_end + 1] == '\r' ? 4 : 2));

        // Parse headers
        std::istringstream header_stream(header_section);
        std::string line;
        bool first_line = true;

        while (std::getline(header_stream, line)) {
            // Skip HTTP status line
            if (first_line) {
                first_line = false;
                continue;
            }

            // Remove \r if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            // Parse header line (key: value)
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string key = line.substr(0, colon);
                std::string value = line.substr(colon + 1);

                // Trim whitespace from value
                size_t value_start = value.find_first_not_of(" \t");
                if (value_start != std::string::npos) {
                    value = value.substr(value_start);
                }

                // Convert key to lowercase for case-insensitive lookup
                for (char& c : key) {
                    c = std::tolower(static_cast<unsigned char>(c));
                }

                response.headers[key] = value;
            }
        }
    } else {
        // No header/body separation found, treat all as body
        response.body = full_response;
    }

    return response;
}

} // namespace kyros
