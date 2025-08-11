#pragma once

#include "LocationConfig.hpp"
#include <string>
#include <vector>
#include <map>

class ServerConfig {
public:
	// --- Constructors and Destructor ---
	ServerConfig();
	~ServerConfig();
	ServerConfig(const ServerConfig& other);
	ServerConfig& operator=(const ServerConfig& other);

	// --- Public Member Variables ---
	// Fields are public or have getters for easy access by the request handler.
	
	int getPort() const;
	void setPort(int port);
	// The port number this server will listen on.
	int _port;

	// The host address for the server to bind to (e.g., "127.0.0.1" or "0.0.0.0").
	std::string _host;

	// A list of server names (domains) that this server block will respond to.
	std::vector<std::string> _server_names;

	// A map of HTTP status codes to custom error page paths.
	std::map<int, std::string> _error_pages;

	// The maximum allowed size for a client's request body, in bytes.
	size_t _client_max_body_size;

	// A collection of LocationConfig objects for the routes defined within this server block.
	std::vector<LocationConfig> _locations;

	// --- Public Methods ---
	// Finds the most specific LocationConfig that matches a given request URI.
	const LocationConfig& findLocationForRequest(const std::string& uri_path) const;
};
