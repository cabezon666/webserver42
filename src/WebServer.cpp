/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vflores- <vflores-@student.42luxembou      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/07 15:23:16 by vflores-          #+#    #+#             */
/*   Updated: 2025/08/07 16:22:57 by vflores-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/WebServer.hpp"
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <arpa/inet.h>

WebServer::WebServer(const std::vector<ServerConfig>& servers)
	: _servers(servers) {}

WebServer::~WebServer() {
	for (size_t i = 0; i < _poll_fds.size(); i++) {
		close (_poll_fds[i].fd);
	}
}

void WebServer::setupSockets() {
	for (size_t i = 0; i < _servers.size(); i++) {
		int server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd < 0) {
			perror("socket");
			continue;
		}

		int opt = 1;
		setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		fcntl(server_fd, F_SETFL, O_NONBLOCK);

		sockaddr_in addr;
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr("0.0.0.0");
		addr.sin_port = htons(_servers[i].getPort());

		if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			perror("bind");
			close(server_fd);
			continue;
		}

		if (listen(server_fd, 10) < 0) {
			perror("listen");
			close(server_fd);
			continue;
		}

		struct pollfd pfd;
		pfd.fd = server_fd;
		pfd.events = POLLIN;
		_poll_fds.push_back(pfd);
		_server_fds.push_back(server_fd);

		std::cout << "Listening on port: " << _servers[i].getPort() << std::endl;
	}
}

void WebServer::run() {
	setupSockets();
	mainLoop();
}

void WebServer::mainLoop() {
	while (true) {
		int activity = poll(_poll_fds.data(), _poll_fds.size(), -1);
		if (activity < 0) {
			perror("poll");
			break;
		}

		for (size_t i = 0; i < _poll_fds.size(); i++) {
			if (_poll_fds[i].revents & POLLIN) {
				if (std::find(_server_fds.begin(), _server_fds.end(), _poll_fds[i].fd) != _server_fds.end()) {
					acceptNewConnection(_poll_fds[i].fd);
				} else {
					handleClientData(_poll_fds[i].fd);
				}
			}
		}
	}
}

void WebServer::acceptNewConnection(int server_fd) {
	int client_fd = accept(server_fd, NULL, NULL);
	if (client_fd < 0) {
		perror("accept");
		return;
	}

	fcntl(client_fd, F_SETFL, O_NONBLOCK);
	struct pollfd client_pfd;
	client_pfd.fd = client_fd;
	client_pfd.events = POLLIN;
	_poll_fds.push_back(client_pfd);

	std::cout << "New client connected: " << client_fd << std::endl;
}

void WebServer::handleClientData(int client_fd) {
	char buffer[1024];
	ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	if (bytes <= 0) {
		std::cout << "Client disconnected: " << client_fd << std::endl;
		close(client_fd);
		removeClient(client_fd);
		return;
	}

	buffer[bytes] = '\0';
	std::cout << "Received request:\n" << buffer << std::endl;

	std::string response = 
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: 12\r\n"
		"\r\n"
		"Hello World";

	send(client_fd, response.c_str(), response.length(), 0);
}

void WebServer::removeClient(int client_fd) {
	for (std::vector<struct pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end(); it++) {
		if (it->fd == client_fd) {
			_poll_fds.erase(it);
			break;
		}
	}
}
