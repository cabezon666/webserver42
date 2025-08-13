/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ewiese-m <ewiese-m@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/01 15:38:23 by ewiese-m          #+#    #+#             */
/*   Updated: 2025/08/13 15:38:30 by ewiese-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Config.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <cctype>

Config::Config(const std::string &file_path) : _servers() {
    parse(file_path);
}

Config::~Config() {}

const std::vector<ServerConfig> &Config::getServers() const {
    return _servers;
}

// Helper function to trim whitespace
static std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Helper function to remove comments
static std::string removeComment(const std::string &line) {
    size_t pos = line.find('#');
    if (pos != std::string::npos) {
        return line.substr(0, pos);
    }
    return line;
}

// Anonymous namespace for helper functions
namespace {
    // Parse methods from string like "GET POST DELETE"
    std::vector<std::string> parseMethods(const std::string &methods_str) {
        std::vector<std::string> methods;
        std::istringstream iss(methods_str);
        std::string method;

        while (iss >> method) {
            methods.push_back(method);
        }

        // If no methods specified, default to GET
        if (methods.empty()) {
            methods.push_back("GET");
        }

        return methods;
    }
}

void Config::parse(const std::string &config_file) {
    std::ifstream file(config_file.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + config_file);
    }

    std::string line;
    while (std::getline(file, line)) {
        line = removeComment(line);
        line = trim(line);

        if (line.empty()) continue;

        // Check for server block
        if (line == "server {" || line == "server{") {
            bool closed = false;
            parseServerBlock(file, &closed);
            if (!closed) {
                throw std::runtime_error("Unclosed server block");
            }
        } else if (line.find("server") == 0) {
            // Handle "server {" on same line or next line
            std::string next_line;
            if (std::getline(file, next_line)) {
                next_line = trim(removeComment(next_line));
                if (next_line == "{") {
                    bool closed = false;
                    parseServerBlock(file, &closed);
                    if (!closed) {
                        throw std::runtime_error("Unclosed server block");
                    }
                }
            }
        }
    }

    if (_servers.empty()) {
        throw std::runtime_error("No server blocks found in configuration");
    }
}

void Config::parseServerBlock(std::istream &file, bool *closed) {
    _servers.push_back(ServerConfig());
    ServerConfig &server = _servers.back();

    // Set defaults
    server._port = 80;
    server._host = "0.0.0.0";
    server._client_max_body_size = 1048576; // 1MB default

    std::string line;
    LocationConfig current_location;
    bool in_location = false;

    while (std::getline(file, line)) {
        line = removeComment(line);
        line = trim(line);

        if (line.empty()) continue;

        // Check for closing brace
        if (line == "}") {
            if (in_location) {
                // Close location block
                if (current_location._root.empty() && !server._locations.empty()) {
                    // Inherit root from server if not specified
                    for (size_t i = 0; i < server._locations.size(); i++) {
                        if (server._locations[i]._path == "/") {
                            current_location._root = server._locations[i]._root;
                            break;
                        }
                    }
                }
                server._locations.push_back(current_location);
                current_location = LocationConfig();
                in_location = false;
            } else {
                // Close server block
                *closed = true;
                break;
            }
            continue;
        }

        // Parse directives
        std::istringstream iss(line);
        std::string directive;
        iss >> directive;

        if (directive == "location") {
            if (in_location) {
                // Save previous location
                server._locations.push_back(current_location);
            }

            // Start new location
            current_location = LocationConfig();
            std::string path;
            iss >> path;

            // Remove trailing {
            if (!path.empty() && path[path.length() - 1] == '{') {
                path = path.substr(0, path.length() - 1);
                path = trim(path);
            }

            current_location._path = path;
            in_location = true;

            // Check if { is on same line
            std::string rest;
            std::getline(iss, rest);
            rest = trim(rest);

            // Look for opening brace
            if (rest != "{" && rest != "") {
                // Next line should have the brace
                std::getline(file, line);
                line = removeComment(line);
                line = trim(line);
                if (line != "{") {
                    throw std::runtime_error("Expected '{' after location directive");
                }
            }
        } else {
            std::string value;
            std::getline(iss, value);
            value = trim(value);

            if (in_location) {
                parseLocationDirective(current_location, directive, value);
            } else {
                parseServerDirective(server, directive, value);
            }
        }
    }

    // Add any remaining location
    if (in_location) {
        server._locations.push_back(current_location);
    }

    // Ensure default location exists
    bool has_default = false;
    for (size_t i = 0; i < server._locations.size(); i++) {
        if (server._locations[i]._path == "/") {
            has_default = true;
            break;
        }
    }

    if (!has_default && !server._locations.empty()) {
        // Create default location from first location or defaults
        LocationConfig default_loc;
        default_loc._path = "/";
        default_loc._root = "www";
        default_loc._index_file = "index.html";
        default_loc._directory_listing = false;
        default_loc._allowed_methods.push_back("GET");
        server._locations.insert(server._locations.begin(), default_loc);
    }
}

void Config::parseServerDirective(ServerConfig &server, const std::string &directive,
                                  const std::string &value) {
    if (directive == "listen") {
        // Handle port parsing
        server._port = atoi(value.c_str());
    } else if (directive == "host") {
        server._host = value;
    } else if (directive == "server_name") {
        // Parse multiple server names
        std::istringstream iss(value);
        std::string name;
        while (iss >> name) {
            server._server_names.push_back(name);
        }
    } else if (directive == "client_max_body_size") {
        // Parse size (remove comments if any)
        std::istringstream iss(value);
        size_t size;
        iss >> size;
        server._client_max_body_size = size;
    } else if (directive == "error_page") {
        // Parse error page: "404 /error/404.html"
        std::istringstream iss(value);
        int code;
        std::string path;
        iss >> code >> path;
        server._error_pages[code] = path;
    } else if (directive == "root") {
        // Default root for server
        // This would be used as fallback, but each location handles its own root
    } else if (directive == "index") {
        // Default index for server
        // This would be used as fallback, but each location handles its own index
    }
}

void Config::parseLocationDirective(LocationConfig &location, const std::string &directive,
                                    const std::string &value) {
    if (directive == "root") {
        location._root = value;
    } else if (directive == "index") {
        location._index_file = value;
    } else if (directive == "autoindex") {
        location._directory_listing = (value == "on");
    } else if (directive == "allow") {
        // Parse allowed methods
        location._allowed_methods = parseMethods(value);
    } else if (directive == "allow_methods") {
        // Alternative syntax for allowed methods
        location._allowed_methods = parseMethods(value);
    } else if (directive == "cgi_path") {
        location._cgi_path = value;
    } else if (directive == "cgi_extension" || directive == "cgi_ext") {
        location._cgi_extension = value;
    } else if (directive == "upload_path") {
        location._upload_path = value;
    } else if (directive == "return") {
        location._redirect = value;
    } else if (directive == "client_max_body_size") {
        // Location-specific body size limit
        // This would need to be added to LocationConfig structure
        // For now, we'll ignore it at location level
    } else if (directive == "alias") {
        // Alias functionality - similar to root but replaces location path
        // Would need additional handling in request processing
        location._root = value;
    }
}
