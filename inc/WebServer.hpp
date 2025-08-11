/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vflores- <vflores-@student.42luxembou      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/07 15:51:16 by vflores-          #+#    #+#             */
/*   Updated: 2025/08/07 15:51:33 by vflores-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <vector>
#include <poll.h>
#include <string>
#include <netinet/in.h>
#include "ServerConfig.hpp"

class WebServer {
public:
    WebServer(const std::vector<ServerConfig>& servers);
    ~WebServer();

    void run();

private:
    void setupSockets();
    void mainLoop();
    void acceptNewConnection(int server_fd);
    void handleClientData(int client_fd);
    void removeClient(int client_fd);

    std::vector<ServerConfig> _servers;
    std::vector<struct pollfd> _poll_fds;
    std::vector<int> _server_fds;
};

#endif // WEBSERVER_HPP

