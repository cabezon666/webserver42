/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: webserv <webserv@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/07 15:51:16 by vflores-          #+#    #+#             */
/*   Updated: 2025/08/11 15:00:00 by webserv         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <vector>
#include <poll.h>
#include <string>
#include <netinet/in.h>
#include "ServerConfig.hpp"

// Forward declarations
class HttpRequest;
class HttpResponse;
class LocationConfig;
struct ClientConnection;

class WebServer {
public:
    // Constructor and Destructor
    WebServer(const std::vector<ServerConfig>& servers);
    ~WebServer();

    // Main run method
    void run();

private:
    // Setup and main loop
    void setupSockets();
    void mainLoop();

    // Connection management
    void acceptNewConnection(int server_fd);
    void handleClientData(int client_fd);
    void removeClient(int client_fd);
    void checkTimeouts();

    // Request processing
    bool isCompleteRequest(const std::string& buffer);
    void processRequest(ClientConnection& conn);

    // HTTP method handlers
    void handleGetRequest(ClientConnection& conn,
                         const HttpRequest& request,
                         const LocationConfig& location);
    void handlePostRequest(ClientConnection& conn,
                          const HttpRequest& request,
                          const LocationConfig& location);
    void handleDeleteRequest(ClientConnection& conn,
                            const HttpRequest& request,
                            const LocationConfig& location);

    // Special handlers
    void handleCGIRequest(ClientConnection& conn,
                         const HttpRequest& request,
                         const LocationConfig& location,
                         const std::string& script_path);
    void handleFileUpload(ClientConnection& conn,
                         const HttpRequest& request,
                         const LocationConfig& location);

    // Response methods
    void serveStaticFile(int client_fd, const std::string& file_path);
    void sendResponse(int client_fd, const HttpResponse& response);
    void sendErrorResponse(int client_fd, int code,
                          const std::string& message,
                          const ServerConfig* server = NULL);
    void sendRedirectResponse(int client_fd, int code,
                             const std::string& location);

    // Utility methods
    static std::string toString(int num);

    // Member variables
    std::vector<ServerConfig> _servers;
    std::vector<struct pollfd> _poll_fds;
    std::vector<int> _server_fds;
};

#endif // WEBSERVER_HPP
