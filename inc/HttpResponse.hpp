#pragma once

#include <string>
#include <map>

class HttpResponse {
public:
    HttpResponse();

    // Core serialization method
    std::string serialize() const;

    // Error handling
    void setError(int code, const std::string& message);

    // Header management
    void addHeader(const std::string& key, const std::string& value);

    // --- Accessors ---
    int getStatusCode() const;
    const std::string& getBody() const;
    const std::string& getConnectionType() const;
    const std::map<std::string, std::string>& getHeaders() const;

    // --- Mutators ---
    void setStatusCode(int code);
    void setBody(const std::string& content);
    void setConnectionType(const std::string& type);

private:
    int status_code_;
    std::string connection_type_;
    std::map<std::string, std::string> headers_;
    std::string body_;

    // Status code to text mapping
    static std::map<int, std::string> initStatusCodes();
    static const std::map<int, std::string> status_messages_;

    // Case normalization for headers
    static std::string toLowerCase(const std::string& str);
};