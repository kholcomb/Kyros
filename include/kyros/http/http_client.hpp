#ifndef KYROS_HTTP_CLIENT_HPP
#define KYROS_HTTP_CLIENT_HPP

#include <chrono>
#include <map>
#include <string>

namespace kyros {

/**
 * HTTP response structure
 */
struct HttpResponse {
    int status_code = 0;
    std::string body;
    std::map<std::string, std::string> headers;
    bool success = false;
    std::string error_message;
};

/**
 * HTTP client for MCP server communication
 */
class HttpClient {
public:
    HttpClient() = default;
    ~HttpClient() = default;

    /**
     * Send HTTP POST request
     * @param url The target URL
     * @param body The request body (usually JSON)
     * @param headers Optional custom headers
     * @param timeout Request timeout in milliseconds
     * @return HttpResponse structure with result
     */
    HttpResponse post(const std::string& url,
                     const std::string& body,
                     const std::map<std::string, std::string>& headers = {},
                     std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

    /**
     * Send HTTP GET request
     * @param url The target URL
     * @param headers Optional custom headers
     * @param timeout Request timeout in milliseconds
     * @return HttpResponse structure with result
     */
    HttpResponse get(const std::string& url,
                    const std::map<std::string, std::string>& headers = {},
                    std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

private:
    // Helper to parse URL into host, port, path
    struct ParsedUrl {
        std::string scheme;   // http or https
        std::string host;
        int port = 80;
        std::string path;
        bool valid = false;
    };

    ParsedUrl parse_url(const std::string& url);
};

} // namespace kyros

#endif // KYROS_HTTP_CLIENT_HPP
