/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: webserv <webserv@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/11 14:00:00 by webserv          #+#    #+#             */
/*   Updated: 2025/08/11 14:00:00 by webserv         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/HttpRequest.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

// Constructor - Initialize empty request
HttpRequest::HttpRequest() :
    method_(""),
    uri_(""),
    http_version_(""),
    headers_(),
    body_(""),
    is_valid_(false),
    body_length_(0) {}

// Main parsing entry point
bool HttpRequest::parse(const std::string& data) {
    clear();

    if (data.empty()) {
        return false;
    }

    // Find the end of headers (double CRLF)
    size_t header_end = data.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        // Try with just LF
        header_end = data.find("\n\n");
        if (header_end == std::string::npos) {
            return false;
        }
    }

    // Split headers and body
    std::string header_section = data.substr(0, header_end);
    std::string body_section = "";

    if (header_end + 4 < data.length()) {
        body_section = data.substr(header_end + 4);
    } else if (header_end + 2 < data.length()) {
        body_section = data.substr(header_end + 2);
    }

    // Parse request line (first line)
    size_t first_line_end = header_section.find("\r\n");
    if (first_line_end == std::string::npos) {
        first_line_end = header_section.find("\n");
    }

    if (first_line_end == std::string::npos) {
        return false;
    }

    std::string request_line = header_section.substr(0, first_line_end);
    if (!parseRequestLine(request_line)) {
        return false;
    }

    // Parse headers
    std::string headers_only = header_section.substr(first_line_end);
    if (!headers_only.empty()) {
        if (headers_only[0] == '\r') headers_only = headers_only.substr(2);
        else if (headers_only[0] == '\n') headers_only = headers_only.substr(1);

        if (!parseHeaders(headers_only)) {
            return false;
        }
    }

    // Parse body if present
    if (!body_section.empty()) {
        parseBody(body_section);
    }

    is_valid_ = true;
    return true;
}

// Process first line of request
bool HttpRequest::parseRequestLine(const std::string& line) {
    std::istringstream iss(line);

    if (!(iss >> method_ >> uri_ >> http_version_)) {
        return false;
    }

    // Convert method to uppercase
    std::transform(method_.begin(), method_.end(), method_.begin(), ::toupper);

    // Validate method
    if (method_ != "GET" && method_ != "POST" && method_ != "DELETE" &&
        method_ != "PUT" && method_ != "HEAD" && method_ != "OPTIONS") {
        return false;
    }

    // Validate HTTP version
    if (http_version_ != "HTTP/1.0" && http_version_ != "HTTP/1.1") {
        return false;
    }

    return true;
}

// Process header section
bool HttpRequest::parseHeaders(const std::string& headers) {
    std::istringstream stream(headers);
    std::string line;

    while (std::getline(stream, line)) {
        // Remove trailing \r if present
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line = line.substr(0, line.length() - 1);
        }

        // Empty line marks end of headers
        if (line.empty()) {
            break;
        }

        // Find colon separator
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) {
            continue; // Skip malformed header
        }

        // Extract and normalize header name and value
        std::string name = toLowerCase(trim(line.substr(0, colon_pos)));
        std::string value = trim(line.substr(colon_pos + 1));

        if (!name.empty() && !value.empty()) {
            headers_[name] = value;
        }
    }

    // Extract content-length if present
    std::map<std::string, std::string>::const_iterator it = headers_.find("content-length");
    if (it != headers_.end()) {
        std::istringstream length_stream(it->second);
        length_stream >> body_length_;
    }

    return true;
}

// Extract body content
void HttpRequest::parseBody(const std::string& body) {
    body_ = body;

    // If content-length was specified, truncate body to that length
    if (body_length_ > 0 && body_.length() > body_length_) {
        body_ = body_.substr(0, body_length_);
    }
}

// Reset all fields
void HttpRequest::clear() {
    method_ = "";
    uri_ = "";
    http_version_ = "";
    headers_.clear();
    body_ = "";
    is_valid_ = false;
    body_length_ = 0;
}

// --- Accessors ---

const std::string& HttpRequest::getMethod() const {
    return method_;
}

const std::string& HttpRequest::getUri() const {
    return uri_;
}

const std::string& HttpRequest::getHttpVersion() const {
    return http_version_;
}

const std::string& HttpRequest::getBody() const {
    return body_;
}

std::string HttpRequest::getHeader(const std::string& name) const {
    std::string lower_name = toLowerCase(name);
    std::map<std::string, std::string>::const_iterator it = headers_.find(lower_name);

    if (it != headers_.end()) {
        return it->second;
    }
    return "";
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const {
    return headers_;
}

// --- Utility functions ---

std::string HttpRequest::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }

    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string HttpRequest::toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}
