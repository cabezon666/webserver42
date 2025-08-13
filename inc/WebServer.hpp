/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ewiese-m <ewiese-m@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/01 15:37:57 by ewiese-m          #+#    #+#             */
/*   Updated: 2025/08/13 22:41:49 by ewiese-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERVER_HPP
# define WEBSERVER_HPP

# include "ServerConfig.hpp"
# include <netinet/in.h>
# include <poll.h>
# include <string>
# include <vector>

class	HttpRequest;
class	HttpResponse;
class	LocationConfig;
struct ClientConnection;

class WebServer
{
  public:
	WebServer(const std::vector<ServerConfig> &servers);
	~WebServer();
	void run();

  private:
	void setupSockets();
	void mainLoop();
	void acceptNewConnection(int server_fd);
	void handleClientData(int client_fd);
	void removeClient(int client_fd);
	void checkTimeouts();
	bool isCompleteRequest(const std::string &buffer);
	void processRequest(ClientConnection &conn);
	void handleGetRequest(ClientConnection &conn, const HttpRequest &request,
		const LocationConfig &location);
	void handlePostRequest(ClientConnection &conn, const HttpRequest &request,
		const LocationConfig &location);
	void handlePutRequest(ClientConnection &conn, const HttpRequest &request,
		const LocationConfig &location);
	void handleDeleteRequest(ClientConnection &conn, const HttpRequest &request,
		const LocationConfig &location);
	void handleCGIRequest(ClientConnection &conn, const HttpRequest &request,
		const LocationConfig &location, const std::string &script_path);
	void handleFileUpload(ClientConnection &conn, const HttpRequest &request,
		const LocationConfig &location);
	void serveStaticFile(int client_fd, const std::string &file_path,
		bool head_only = false);
	void sendResponse(int client_fd, const HttpResponse &response);
	void sendErrorResponse(int client_fd, int code, const std::string &message,
		const ServerConfig *server = NULL);
	void sendRedirectResponse(int client_fd, int code,
		const std::string &location);
	static std::string toString(int num);
	std::vector<ServerConfig> _servers;
	std::vector<struct pollfd> _poll_fds;
	std::vector<int> _server_fds;
};

#endif
