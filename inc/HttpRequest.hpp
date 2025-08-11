#pragma once

#include <string>
#include <map>

class HttpRequest {
public:
    // Initialize empty request with default values
    HttpRequest();

    // Main parsing entry point. Returns true if valid HTTP request
    // Processes raw data and populates object fields
    bool parse(const std::string& data);

    // --- Accessors ---

    // Get HTTP method (e.g. "GET", "POST")
    const std::string& getMethod() const;

    // Get request URI (e.g. "/index.html")
    const std::string& getUri() const;

    // Get HTTP version (e.g. "HTTP/1.1")
    const std::string& getHttpVersion() const;

    // Get request body content
    const std::string& getBody() const;

    // Get specific header value by name (case-insensitive lookup)
    // Returns empty string if header not found
    std::string getHeader(const std::string& name) const;

    // Get all headers as case-normalized (lowercase) map
    const std::map<std::string, std::string>& getHeaders() const;

private:
    std::string method_;       // HTTP verb (UPPERCASE)
    std::string uri_;          // Resource identifier
    std::string http_version_; // HTTP version string
    std::map<std::string, std::string> headers_;  // Normalized to lowercase keys
    std::string body_;         // Content payload
    bool is_valid_;            // Internal validation state
    size_t body_length_;       // Track expected body size

    // Reset all fields to initial state
    void clear();

    // Process first line of request (e.g. "GET / HTTP/1.1")
    // Returns false on format errors
    bool parseRequestLine(const std::string& line);

    // Process header section line by line
    // Handles header folding and validation
    bool parseHeaders(const std::string& headers);

    // Extract body based on content-length or connection close
    void parseBody(const std::string& body);

    // --- Utilities ---

    // Remove whitespace from both ends of string
    static std::string trim(const std::string& str);

    // Convert string to lowercase (for header normalization)
    static std::string toLowerCase(const std::string& str);
};