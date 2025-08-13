/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ewiese-m <ewiese-m@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/01 15:38:11 by ewiese-m          #+#    #+#             */
/*   Updated: 2025/08/13 15:38:17 by ewiese-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/CGI.hpp"
#include "../inc/HttpRequest.hpp"
#include "../inc/LocationConfig.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <sstream>
#include <fcntl.h>
#include <signal.h>
#include <iostream>
#include <cctype>

// Constructor
CGI::CGI(const HttpRequest& request, const LocationConfig& location) :
    request_(request),
    location_(location),
    env_vars_(),
    timeout_seconds_(30) {

    setupEnvironment();
}

// Destructor
CGI::~CGI() {
    // Clean up environment variables
    for (size_t i = 0; i < env_vars_.size(); ++i) {
        delete[] env_vars_[i];
    }
}

// Main execution method
std::string CGI::execute(const std::string& script_path) {
    // Create pipes for communication
    int pipe_in[2];   // For sending data to CGI
    int pipe_out[2];  // For receiving data from CGI

    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
        return generateErrorResponse(500, "Failed to create pipes");
    }

    // Fork process
    pid_t pid = fork();

    if (pid == -1) {
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);
        return generateErrorResponse(500, "Failed to fork process");
    }

    if (pid == 0) {
        // Child process - execute CGI
        executeCGIChild(script_path, pipe_in, pipe_out);
        // If we reach here, exec failed
        exit(1);
    } else {
        // Parent process - handle I/O
        return handleCGIParent(pid, pipe_in, pipe_out);
    }
}

// Setup CGI environment variables
void CGI::setupEnvironment() {
    env_map_.clear();

    // Set required CGI environment variables
    env_map_["REQUEST_METHOD"] = request_.getMethod();
    env_map_["SERVER_PROTOCOL"] = request_.getHttpVersion();
    env_map_["GATEWAY_INTERFACE"] = "CGI/1.1";
    env_map_["SERVER_SOFTWARE"] = "webserv/1.0";
    env_map_["SERVER_NAME"] = "localhost";
    env_map_["SERVER_PORT"] = "8080";

    // Parse query string from URI
    std::string uri = request_.getUri();
    size_t query_pos = uri.find('?');
    if (query_pos != std::string::npos) {
        env_map_["PATH_INFO"] = uri.substr(0, query_pos);
        env_map_["QUERY_STRING"] = uri.substr(query_pos + 1);
    } else {
        env_map_["PATH_INFO"] = uri;
        env_map_["QUERY_STRING"] = "";
    }

    // Add request headers as HTTP_ variables
    const std::map<std::string, std::string>& headers = request_.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {

        std::string env_name = "HTTP_" + toUpperSnakeCase(it->first);
        env_map_[env_name] = it->second;
    }

    // Set content-related variables for POST
    if (request_.getMethod() == "POST") {
        std::string content_type = request_.getHeader("content-type");
        std::string content_length = request_.getHeader("content-length");

        if (!content_type.empty()) {
            env_map_["CONTENT_TYPE"] = content_type;
        }
        if (!content_length.empty()) {
            env_map_["CONTENT_LENGTH"] = content_length;
        }
    }

    // Convert map to char** for execve
    buildEnvArray();
}

// Build environment array for execve
void CGI::buildEnvArray() {
    // Clear previous array
    for (size_t i = 0; i < env_vars_.size(); ++i) {
        delete[] env_vars_[i];
    }
    env_vars_.clear();

    // Build new array
    for (std::map<std::string, std::string>::const_iterator it = env_map_.begin();
         it != env_map_.end(); ++it) {

        std::string env_str = it->first + "=" + it->second;
        char* env_var = new char[env_str.length() + 1];
        std::strcpy(env_var, env_str.c_str());
        env_vars_.push_back(env_var);
    }

    // Add null terminator
    env_vars_.push_back(NULL);
}

// Execute CGI in child process
void CGI::executeCGIChild(const std::string& script_path, int pipe_in[2], int pipe_out[2]) {
    // Redirect stdin from pipe_in
    close(pipe_in[1]);  // Close write end
    dup2(pipe_in[0], STDIN_FILENO);
    close(pipe_in[0]);

    // Redirect stdout to pipe_out
    close(pipe_out[0]);  // Close read end
    dup2(pipe_out[1], STDOUT_FILENO);
    close(pipe_out[1]);

    // Change to script directory
    std::string directory = getDirectoryPath(script_path);
    if (!directory.empty()) {
        chdir(directory.c_str());
    }

    // Prepare arguments
    const char* argv[3];
    argv[0] = location_._cgi_path.c_str();  // Interpreter path
    argv[1] = script_path.c_str();          // Script path
    argv[2] = NULL;

    // Execute CGI
    execve(argv[0], const_cast<char**>(argv), &env_vars_[0]);

    // If we reach here, exec failed
    std::cerr << "CGI execution failed: " << strerror(errno) << std::endl;
    exit(1);
}

// Handle CGI in parent process
std::string CGI::handleCGIParent(pid_t pid, int pipe_in[2], int pipe_out[2]) {
    // Close unused ends
    close(pipe_in[0]);   // Close read end of input pipe
    close(pipe_out[1]);  // Close write end of output pipe

    // Send POST data if present
    if (request_.getMethod() == "POST" && !request_.getBody().empty()) {
        write(pipe_in[1], request_.getBody().c_str(), request_.getBody().length());
    }
    close(pipe_in[1]);  // Signal EOF to CGI

    // Set non-blocking mode for output pipe
    int flags = fcntl(pipe_out[0], F_GETFL, 0);
    fcntl(pipe_out[0], F_SETFL, flags | O_NONBLOCK);

    // Read CGI output with timeout
    std::string output;
    char buffer[4096];
    time_t start_time = time(NULL);
    int status;

    while (true) {
        // Check timeout
        if (time(NULL) - start_time > timeout_seconds_) {
            kill(pid, SIGTERM);
            waitpid(pid, &status, 0);
            close(pipe_out[0]);
            return generateErrorResponse(504, "CGI timeout");
        }

        // Check if process finished
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid) {
            // Process finished, read remaining output
            ssize_t bytes;
            while ((bytes = read(pipe_out[0], buffer, sizeof(buffer))) > 0) {
                output.append(buffer, bytes);
            }
            break;
        }

        // Try to read output
        ssize_t bytes = read(pipe_out[0], buffer, sizeof(buffer));
        if (bytes > 0) {
            output.append(buffer, bytes);
        } else if (bytes == 0) {
            // EOF reached
            break;
        }

        // Small delay to avoid busy waiting
        usleep(10000);  // 10ms
    }

    close(pipe_out[0]);

    // Check exit status
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        return generateErrorResponse(500, "CGI script error");
    }

    return parseCGIOutput(output);
}

// Parse CGI output and format response
std::string CGI::parseCGIOutput(const std::string& raw_output) {
    if (raw_output.empty()) {
        return generateErrorResponse(500, "Empty CGI response");
    }

    // Find headers/body separator
    size_t separator = raw_output.find("\r\n\r\n");
    if (separator == std::string::npos) {
        separator = raw_output.find("\n\n");
        if (separator == std::string::npos) {
            // No headers, treat all as body
            return "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: " + toString(raw_output.length()) + "\r\n"
                   "Connection: close\r\n"
                   "\r\n" + raw_output;
        }
    }

    // Separate headers and body
    std::string headers = raw_output.substr(0, separator);
    std::string body = raw_output.substr(separator + (raw_output[separator] == '\r' ? 4 : 2));

    // Build response
    std::ostringstream response;

    // Check if Status header is present
    if (headers.find("Status:") != std::string::npos) {
        // Extract status code
        size_t status_pos = headers.find("Status:");
        size_t status_end = headers.find('\n', status_pos);
        std::string status_line = headers.substr(status_pos + 7, status_end - status_pos - 7);
        response << "HTTP/1.1 " << status_line << "\r\n";
    } else {
        response << "HTTP/1.1 200 OK\r\n";
    }

    // Add CGI headers
    response << headers;
    if (headers[headers.length() - 1] != '\n') {
        response << "\r\n";
    }

    // Add Content-Length if not present
    if (headers.find("Content-Length:") == std::string::npos &&
        headers.find("content-length:") == std::string::npos) {
        response << "Content-Length: " << body.length() << "\r\n";
    }

    // Add server header
    response << "Server: webserv/1.0\r\n";
    response << "\r\n";

    // Add body
    response << body;

    return response.str();
}

// Generate error response
std::string CGI::generateErrorResponse(int code, const std::string& message) {
    std::ostringstream response;

    std::string body = "<!DOCTYPE html>\n"
                      "<html>\n"
                      "<head><title>CGI Error</title></head>\n"
                      "<body>\n"
                      "<h1>Error " + toString(code) + "</h1>\n"
                      "<p>" + message + "</p>\n"
                      "</body>\n"
                      "</html>\n";

    response << "HTTP/1.1 " << code << " " << getStatusMessage(code) << "\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;

    return response.str();
}

// Get directory path from file path
std::string CGI::getDirectoryPath(const std::string& file_path) {
    size_t last_slash = file_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        return file_path.substr(0, last_slash);
    }
    return "";
}

// Convert header name to uppercase snake case
std::string CGI::toUpperSnakeCase(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '-') {
            result += '_';
        } else {
            result += std::toupper(str[i]);
        }
    }
    return result;
}

// Convert integer to string
std::string CGI::toString(int num) {
    std::ostringstream oss;
    oss << num;
    return oss.str();
}

// Get status message for code
std::string CGI::getStatusMessage(int code) {
    switch (code) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        case 504: return "Gateway Timeout";
        default: return "Error";
    }
}
