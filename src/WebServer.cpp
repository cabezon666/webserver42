/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: webserv <webserv@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/07 15:23:16 by vflores-          #+#    #+#             */
/*   Updated: 2025/08/11 15:00:00 by webserv         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/WebServer.hpp"
#include "../inc/HttpRequest.hpp"
#include "../inc/HttpResponse.hpp"
#include "../inc/CGI.hpp"
#include "../inc/utils.hpp"
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <arpa/inet.h>
#include <sstream>
#include <map>
#include <cerrno>

// Client connection structure
struct ClientConnection {
    int fd;
    std::string buffer;
    time_t last_activity;
    bool keep_alive;
    const ServerConfig* server;
};

// Static member for client connections
static std::map<int, ClientConnection> g_clients;
static const int BUFFER_SIZE = 8192;
static const int TIMEOUT_SECONDS = 30;

WebServer::WebServer(const std::vector<ServerConfig>& servers)
    : _servers(servers) {}

WebServer::~WebServer() {
    // Close all sockets
    for (size_t i = 0; i < _poll_fds.size(); i++) {
        close(_poll_fds[i].fd);
    }
    g_clients.clear();
}

void WebServer::setupSockets() {
    // Map to track unique host:port combinations
    std::map<std::string, int> used_addresses;

    for (size_t i = 0; i < _servers.size(); i++) {
        // Create address key
        std::ostringstream addr_key;
        addr_key << _servers[i]._host << ":" << _servers[i].getPort();

        // Check if already bound
        if (used_addresses.find(addr_key.str()) != used_addresses.end()) {
            std::cout << "Already listening on " << addr_key.str() << std::endl;
            continue;
        }

        // Create socket
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            perror("socket");
            continue;
        }

        // Set socket options
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt");
            close(server_fd);
            continue;
        }

        // Set non-blocking
        fcntl(server_fd, F_SETFL, O_NONBLOCK);

        // Prepare address structure
        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;

        // Convert host to network address
        if (_servers[i]._host == "0.0.0.0" || _servers[i]._host.empty()) {
            addr.sin_addr.s_addr = INADDR_ANY;
        } else {
            if (inet_pton(AF_INET, _servers[i]._host.c_str(), &addr.sin_addr) <= 0) {
                std::cerr << "Invalid address: " << _servers[i]._host << std::endl;
                close(server_fd);
                continue;
            }
        }

        addr.sin_port = htons(_servers[i].getPort());

        // Bind socket
        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind");
            close(server_fd);
            continue;
        }

        // Listen for connections
        if (listen(server_fd, 128) < 0) {
            perror("listen");
            close(server_fd);
            continue;
        }

        // Add to poll structure
        struct pollfd pfd;
        pfd.fd = server_fd;
        pfd.events = POLLIN;
        _poll_fds.push_back(pfd);
        _server_fds.push_back(server_fd);

        // Mark as used
        used_addresses[addr_key.str()] = server_fd;

        std::cout << "✓ Listening on " << _servers[i]._host
                  << ":" << _servers[i].getPort() << std::endl;
    }

    if (_server_fds.empty()) {
        throw std::runtime_error("Failed to bind any server socket");
    }
}

void WebServer::run() {
    setupSockets();
    std::cout << "\n🚀 Webserv started successfully!\n" << std::endl;
    mainLoop();
}

void WebServer::mainLoop() {
    while (true) {
        // Check for timeouts
        checkTimeouts();

        // Poll for events
        int activity = poll(_poll_fds.data(), _poll_fds.size(), 1000);

        if (activity < 0) {
            if (errno != EINTR) {
                perror("poll");
                break;
            }
            continue;
        }

        // Process events
        for (size_t i = 0; i < _poll_fds.size(); i++) {
            if (_poll_fds[i].revents & POLLIN) {
                // Check if it's a server socket
                if (std::find(_server_fds.begin(), _server_fds.end(),
                             _poll_fds[i].fd) != _server_fds.end()) {
                    acceptNewConnection(_poll_fds[i].fd);
                } else {
                    handleClientData(_poll_fds[i].fd);
                }
            } else if (_poll_fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                // Connection closed or error
                if (std::find(_server_fds.begin(), _server_fds.end(),
                             _poll_fds[i].fd) == _server_fds.end()) {
                    std::cout << "Client disconnected: " << _poll_fds[i].fd << std::endl;
                    removeClient(_poll_fds[i].fd);
                }
            }
        }
    }
}

void WebServer::acceptNewConnection(int server_fd) {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }
        return;
    }

    // Set non-blocking
    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    // Add to poll structure
    struct pollfd client_pfd;
    client_pfd.fd = client_fd;
    client_pfd.events = POLLIN;
    _poll_fds.push_back(client_pfd);

    // Create client connection
    ClientConnection conn;
    conn.fd = client_fd;
    conn.buffer = "";
    conn.last_activity = time(NULL);
    conn.keep_alive = false;

    // Find appropriate server config
    conn.server = &_servers[0];  // Default to first server
    for (size_t i = 0; i < _servers.size(); i++) {
        // Match by port for now (can be enhanced with Host header later)
        sockaddr_in local_addr;
        socklen_t local_len = sizeof(local_addr);
        getsockname(server_fd, (struct sockaddr*)&local_addr, &local_len);

        if (_servers[i].getPort() == ntohs(local_addr.sin_port)) {
            conn.server = &_servers[i];
            break;
        }
    }

    g_clients[client_fd] = conn;

    // Get client IP
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

    std::cout << "✓ New client connected: " << client_ip
              << " (fd: " << client_fd << ")" << std::endl;
}

void WebServer::handleClientData(int client_fd) {
    // Find client connection
    std::map<int, ClientConnection>::iterator it = g_clients.find(client_fd);
    if (it == g_clients.end()) {
        return;
    }

    ClientConnection& conn = it->second;
    conn.last_activity = time(NULL);

    // Read data
    char buffer[BUFFER_SIZE];
    ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes <= 0) {
        if (bytes == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
            std::cout << "Client disconnected: " << client_fd << std::endl;
            removeClient(client_fd);
        }
        return;
    }

    buffer[bytes] = '\0';
    conn.buffer.append(buffer, bytes);

    // Check if we have a complete request
    if (isCompleteRequest(conn.buffer)) {
        processRequest(conn);

        // Clear buffer for next request
        conn.buffer.clear();

        // Close connection if not keep-alive
        if (!conn.keep_alive) {
            removeClient(client_fd);
        }
    } else if (conn.buffer.size() > 1024 * 1024) {
        // Request too large (> 1MB)
        sendErrorResponse(client_fd, 413, "Payload Too Large");
        removeClient(client_fd);
    }
}

bool WebServer::isCompleteRequest(const std::string& buffer) {
    // Check for end of headers
    if (buffer.find("\r\n\r\n") == std::string::npos &&
        buffer.find("\n\n") == std::string::npos) {
        return false;
    }

    // Check if we need to wait for body
    size_t content_length = 0;
    size_t pos = buffer.find("Content-Length:");
    if (pos == std::string::npos) {
        pos = buffer.find("content-length:");
    }

    if (pos != std::string::npos) {
        size_t end = buffer.find("\r\n", pos);
        if (end == std::string::npos) {
            end = buffer.find("\n", pos);
        }

        if (end != std::string::npos) {
            std::string length_str = buffer.substr(pos + 15, end - pos - 15);
            std::istringstream iss(length_str);
            iss >> content_length;

            // Find body start
            size_t body_start = buffer.find("\r\n\r\n");
            if (body_start == std::string::npos) {
                body_start = buffer.find("\n\n");
                if (body_start != std::string::npos) {
                    body_start += 2;
                }
            } else {
                body_start += 4;
            }

            if (body_start != std::string::npos) {
                size_t body_length = buffer.length() - body_start;
                return body_length >= content_length;
            }
        }
    }

    return true;
}

void WebServer::processRequest(ClientConnection& conn) {
    // Parse HTTP request
    HttpRequest request;
    if (!request.parse(conn.buffer)) {
        sendErrorResponse(conn.fd, 400, "Bad Request");
        return;
    }

    std::cout << "📥 " << request.getMethod() << " " << request.getUri()
              << " from fd:" << conn.fd << std::endl;

    // Check connection header
    std::string connection = request.getHeader("connection");
    conn.keep_alive = (request.getHttpVersion() == "HTTP/1.1" &&
                      toLowerCase(connection) != "close") ||
                     (toLowerCase(connection) == "keep-alive");

    // Find matching location
    const LocationConfig& location = conn.server->findLocationForRequest(request.getUri());

    // Check if method is allowed
    if (!location._allowed_methods.empty()) {
        bool method_allowed = false;
        for (size_t i = 0; i < location._allowed_methods.size(); i++) {
            if (location._allowed_methods[i] == request.getMethod()) {
                method_allowed = true;
                break;
            }
        }

        if (!method_allowed) {
            sendErrorResponse(conn.fd, 405, "Method Not Allowed");
            return;
        }
    }

    // Handle different methods
    if (request.getMethod() == "GET") {
        handleGetRequest(conn, request, location);
    } else if (request.getMethod() == "POST") {
        handlePostRequest(conn, request, location);
    } else if (request.getMethod() == "DELETE") {
        handleDeleteRequest(conn, request, location);
    } else {
        sendErrorResponse(conn.fd, 501, "Not Implemented");
    }
}

void WebServer::handleGetRequest(ClientConnection& conn,
                                 const HttpRequest& request,
                                 const LocationConfig& location) {
    // Check for redirect
    if (!location._redirect.empty()) {
        sendRedirectResponse(conn.fd, 301, location._redirect);
        return;
    }

    // Build file path
    std::string file_path = location._root;
    if (file_path.empty()) {
        file_path = "./www";
    }

    // Parse URI to remove query string
    std::string uri = request.getUri();
    size_t query_pos = uri.find('?');
    if (query_pos != std::string::npos) {
        uri = uri.substr(0, query_pos);
    }

    // URL decode
    uri = urlDecode(uri);

    // Security: prevent directory traversal
    if (uri.find("../") != std::string::npos) {
        sendErrorResponse(conn.fd, 403, "Forbidden");
        return;
    }

    file_path += uri;

    // Check if path exists
    if (!fileExists(file_path)) {
        sendErrorResponse(conn.fd, 404, "Not Found", conn.server);
        return;
    }

    // If directory, check for index file or directory listing
    if (isDirectory(file_path)) {
        if (file_path[file_path.length() - 1] != '/') {
            // Redirect to path with trailing slash
            sendRedirectResponse(conn.fd, 301, uri + "/");
            return;
        }

        // Try index file
        std::string index_path = file_path + location._index_file;
        if (!location._index_file.empty() && fileExists(index_path)) {
            file_path = index_path;
        } else if (location._directory_listing) {
            // Generate directory listing
            std::string listing = generateDirectoryListing(file_path, uri);
            if (listing.empty()) {
                sendErrorResponse(conn.fd, 403, "Forbidden");
                return;
            }

            HttpResponse response;
            response.setStatusCode(200);
            response.setBody(listing);
            response.addHeader("content-type", "text/html");
            sendResponse(conn.fd, response);
            return;
        } else {
            sendErrorResponse(conn.fd, 403, "Forbidden");
            return;
        }
    }

    // Check if file is readable
    if (!isReadable(file_path)) {
        sendErrorResponse(conn.fd, 403, "Forbidden");
        return;
    }

    // Check for CGI
    if (!location._cgi_extension.empty() &&
        file_path.find(location._cgi_extension) != std::string::npos) {
        handleCGIRequest(conn, request, location, file_path);
        return;
    }

    // Serve static file
    serveStaticFile(conn.fd, file_path);
}

void WebServer::handlePostRequest(ClientConnection& conn,
                                  const HttpRequest& request,
                                  const LocationConfig& location) {
    // Check body size limit
    if (conn.server->_client_max_body_size > 0 &&
        request.getBody().size() > conn.server->_client_max_body_size) {
        sendErrorResponse(conn.fd, 413, "Payload Too Large");
        return;
    }

    // Check for CGI
    std::string uri = request.getUri();
    size_t query_pos = uri.find('?');
    if (query_pos != std::string::npos) {
        uri = uri.substr(0, query_pos);
    }

    std::string file_path = location._root + uri;

    if (!location._cgi_extension.empty() &&
        file_path.find(location._cgi_extension) != std::string::npos) {
        handleCGIRequest(conn, request, location, file_path);
        return;
    }

    // Handle file upload if upload path is configured
    if (!location._upload_path.empty()) {
        handleFileUpload(conn, request, location);
        return;
    }

    // Default: method not allowed for this resource
    sendErrorResponse(conn.fd, 405, "Method Not Allowed");
}

void WebServer::handleDeleteRequest(ClientConnection& conn,
                                    const HttpRequest& request,
                                    const LocationConfig& location) {
    // Build file path
    std::string file_path = location._root;
    if (file_path.empty()) {
        file_path = "./www";
    }

    std::string uri = urlDecode(request.getUri());

    // Security: prevent directory traversal
    if (uri.find("../") != std::string::npos) {
        sendErrorResponse(conn.fd, 403, "Forbidden");
        return;
    }

    file_path += uri;

    // Check if file exists
    if (!fileExists(file_path)) {
        sendErrorResponse(conn.fd, 404, "Not Found");
        return;
    }

    // Don't allow deleting directories
    if (isDirectory(file_path)) {
        sendErrorResponse(conn.fd, 403, "Forbidden");
        return;
    }

    // Check if file is writable
    if (!isWritable(file_path)) {
        sendErrorResponse(conn.fd, 403, "Forbidden");
        return;
    }

    // Delete the file
    if (unlink(file_path.c_str()) == 0) {
        HttpResponse response;
        response.setStatusCode(204);  // No Content
        sendResponse(conn.fd, response);
        std::cout << "🗑️  Deleted: " << file_path << std::endl;
    } else {
        sendErrorResponse(conn.fd, 500, "Internal Server Error");
    }
}

void WebServer::handleCGIRequest(ClientConnection& conn,
                                 const HttpRequest& request,
                                 const LocationConfig& location,
                                 const std::string& script_path) {
    std::cout << "🔧 Executing CGI: " << script_path << std::endl;

    CGI cgi(request, location);
    std::string response = cgi.execute(script_path);

    send(conn.fd, response.c_str(), response.length(), 0);
}

void WebServer::handleFileUpload(ClientConnection& conn,
                                 const HttpRequest& request,
                                 const LocationConfig& location) {
    // Get filename from Content-Disposition header or generate one
    std::string filename = "upload_" + toString(time(NULL));
    std::string content_disp = request.getHeader("content-disposition");

    if (!content_disp.empty()) {
        size_t pos = content_disp.find("filename=");
        if (pos != std::string::npos) {
            pos += 9;
            if (content_disp[pos] == '"') pos++;
            size_t end = content_disp.find('"', pos);
            if (end == std::string::npos) {
                end = content_disp.find(';', pos);
            }
            if (end != std::string::npos) {
                filename = content_disp.substr(pos, end - pos);
            }
        }
    }

    // Build upload path
    std::string upload_path = location._upload_path;
    if (upload_path[upload_path.length() - 1] != '/') {
        upload_path += '/';
    }
    upload_path += filename;

    // Write file
    if (writeFile(upload_path, request.getBody())) {
        HttpResponse response;
        response.setStatusCode(201);  // Created
        response.addHeader("location", "/" + upload_path);
        response.setBody("File uploaded successfully");
        sendResponse(conn.fd, response);
        std::cout << "📤 File uploaded: " << upload_path << std::endl;
    } else {
        sendErrorResponse(conn.fd, 500, "Failed to save file");
    }
}

void WebServer::serveStaticFile(int client_fd, const std::string& file_path) {
    // Read file
    std::string content = readFile(file_path);
    if (content.empty() && getFileSize(file_path) > 0) {
        sendErrorResponse(client_fd, 500, "Failed to read file");
        return;
    }

    // Create response
    HttpResponse response;
    response.setStatusCode(200);
    response.setBody(content);
    response.addHeader("content-type", getMimeType(file_path));

    sendResponse(client_fd, response);
}

void WebServer::sendResponse(int client_fd, const HttpResponse& response) {
    std::string data = response.serialize();
    send(client_fd, data.c_str(), data.length(), 0);
}

void WebServer::sendErrorResponse(int client_fd, int code,
                                  const std::string& message,
                                  const ServerConfig* server) {
    HttpResponse response;

    // Check for custom error page
    if (server) {
        std::map<int, std::string>::const_iterator it = server->_error_pages.find(code);
        if (it != server->_error_pages.end()) {
            std::string error_page = readFile(it->second);
            if (!error_page.empty()) {
                response.setStatusCode(code);
                response.setBody(error_page);
                response.addHeader("content-type", "text/html");
                sendResponse(client_fd, response);
                return;
            }
        }
    }

    // Default error response
    response.setError(code, message);
    sendResponse(client_fd, response);
}

void WebServer::sendRedirectResponse(int client_fd, int code,
                                     const std::string& location) {
    HttpResponse response;
    response.setStatusCode(code);
    response.addHeader("location", location);

    std::string body = "<!DOCTYPE html><html><head><title>Redirect</title></head>"
                      "<body><h1>Redirecting...</h1><p>Redirecting to "
                      "<a href=\"" + location + "\">" + location + "</a></p></body></html>";
    response.setBody(body);
    response.addHeader("content-type", "text/html");

    sendResponse(client_fd, response);
}

void WebServer::removeClient(int client_fd) {
    // Remove from clients map
    g_clients.erase(client_fd);

    // Close socket
    close(client_fd);

    // Remove from poll structure
    for (std::vector<struct pollfd>::iterator it = _poll_fds.begin();
         it != _poll_fds.end(); ++it) {
        if (it->fd == client_fd) {
            _poll_fds.erase(it);
            break;
        }
    }
}

void WebServer::checkTimeouts() {
    time_t now = time(NULL);
    std::vector<int> to_remove;

    for (std::map<int, ClientConnection>::iterator it = g_clients.begin();
         it != g_clients.end(); ++it) {
        if (now - it->second.last_activity > TIMEOUT_SECONDS) {
            to_remove.push_back(it->first);
        }
    }

    for (size_t i = 0; i < to_remove.size(); ++i) {
        std::cout << "⏱️  Timeout: closing connection " << to_remove[i] << std::endl;
        removeClient(to_remove[i]);
    }
}

// Helper function to convert int to string
std::string WebServer::toString(int num) {
    std::ostringstream oss;
    oss << num;
    return oss.str();
}
