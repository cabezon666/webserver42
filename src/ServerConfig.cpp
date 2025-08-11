/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ewiese-m <ewiese-m@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/11 12:47:33 by ewiese-m          #+#    #+#             */
/*   Updated: 2025/08/11 13:35:50 by ewiese-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/ServerConfig.hpp"

// Default constructor
ServerConfig::ServerConfig() :
    _port(80),
    _host("0.0.0.0"),
    _server_names(),
    _error_pages(),
    _client_max_body_size(0),
    _locations() {}

// Destructor - no dynamic resources to clean
ServerConfig::~ServerConfig() {}

// Copy constructor
ServerConfig::ServerConfig(const ServerConfig& other) :
    _port(other._port),
    _host(other._host),
    _server_names(other._server_names),
    _error_pages(other._error_pages),
    _client_max_body_size(other._client_max_body_size),
    _locations(other._locations) {}

// Assignment operator
ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this != &other) {
        _port = other._port;
        _host = other._host;
        _server_names = other._server_names;
        _error_pages = other._error_pages;
        _client_max_body_size = other._client_max_body_size;
        _locations = other._locations;
    }
    return *this;
}

int ServerConfig::getPort() const {
	return _port;
}

void ServerConfig::setPort(int port) {
	_port = port;
}

// Location matching implementation
const LocationConfig& ServerConfig::findLocationForRequest(const std::string& uri_path) const {
    const LocationConfig* best_match = &_locations[0];
    size_t best_length = 0;

    for (std::vector<LocationConfig>::const_iterator it = _locations.begin();
         it != _locations.end(); ++it) {

        const std::string& loc_path = it->_path;
        const size_t loc_len = loc_path.length();

        if ((uri_path == loc_path) ||
            (uri_path.compare(0, loc_len, loc_path) == 0 &&
             (loc_path[loc_path.size() - 1] == '/' || uri_path[loc_len] == '/'))) {

            if (loc_len > best_length) {
                best_length = loc_len;
                best_match = &(*it);
            }
        }
    }

    return *best_match;
}
