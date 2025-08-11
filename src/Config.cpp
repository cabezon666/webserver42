/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ewiese-m <ewiese-m@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/11 12:47:49 by ewiese-m          #+#    #+#             */
/*   Updated: 2025/08/11 21:40:48 by ewiese-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Config.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>

Config::Config(const std::string &file_path) : _servers()
{
	parse(file_path);
}

Config::Config() : _servers()
{
}

Config::~Config()
{
}

Config::Config(const Config &other) : _servers(other._servers)
{
}

Config &Config::operator=(const Config &other)
{
	if (this != &other)
	{
		_servers = other._servers;
	}
	return *this;
}

const std::vector<ServerConfig> &Config::getServers() const
{
	return _servers;
}

void Config::parse(const std::string &config_file)
{
	std::ifstream file(config_file.c_str());
	if (!file.is_open())
	{
		throw std::runtime_error("Could not open config file: " + config_file);
	}

	std::string line;
	while (std::getline(file, line))
	{
		std::istringstream iss(line);
		//line.
		std::string token;
		iss >> token;
		bool closed = false;
		if (token == "server")
		{
			parseServerBlock(file, &closed);
			if (!closed)
				throw std::runtime_error(
					"Bad configuration file format : open brackets on server config");
		}
	}
	if (_servers.empty())
		throw std::runtime_error("No servers in configuration file\n");
}

void Config::parseServerBlock(std::istream &config_stream, bool *closed)
{
	_servers.push_back(ServerConfig());
	ServerConfig &server = _servers.back();
	LocationConfig default_location;

	std::string line;
	while (std::getline(config_stream, line))
	{
		std::istringstream iss(line);
		std::string directive;
		iss >> directive;
		bool location_closed = false;
		std::string v;
		std::getline(iss, v);
		std::cout << line << " | " << directive << ":" << std::istringstream(v) << std::endl;
		if (directive == "location")
		{
			parseLocationBlock(config_stream, server, &location_closed);
			if (location_closed)
				printf("location_closed = true\n");
			if (!location_closed)
				throw std::runtime_error(
					"Bad configuration file format : open brackets on location config");
		} else if (directive == "}")
		{
			*closed = true;
			break;
		} else
		{
			std::string value;
			std::getline(iss, value);
			parseDirective(server, default_location, directive, value);
		}
	}

	server._locations.push_back(default_location);
}

void Config::parseLocationBlock(std::istream &config_stream,
								ServerConfig &server, bool *location_closed)
{
	server._locations.push_back(LocationConfig());
	LocationConfig &location = server._locations.back();

	std::string path;
	std::string open_brace;
	config_stream >> path >> open_brace;
	location._path = path;

	std::string line;
	*location_closed = false;
	while (std::getline(config_stream, line))
	{
		std::istringstream iss(line);
		std::string directive;
		iss >> directive;
		/*
		std::cout << directive << std::endl;
		if (*location_closed)
			std::cout << "location TRUE\n";
		else
			std::cout << "location FALSE\n";
			*/
		if (directive == "}")
		{
			*location_closed = true;
			printf("breaking location \n");
			break;
		}
		std::string value;
		std::getline(iss, value);
		parseDirective(server, location, directive, value);
	}
}

void Config::parseDirective(ServerConfig &server, LocationConfig &location,
							const std::string &directive,
							const std::string &value)
{
	std::istringstream iss(value);
	std::string first;
	iss >> first;

	if (directive == "listen")
	{
		//server._port = atoi(first.c_str());
		size_t colon_pos = first.find(':');
		if (colon_pos != std::string::npos)
			server.setPort(atoi(first.substr(colon_pos + 1).c_str()));
		else
			server.setPort(atoi(first.c_str()));
	} else if (directive == "host")
	{
		server._host = first;
	} else if (directive == "server_name")
	{
		server._server_names.push_back(first);
	} else if (directive == "error_page")
	{
		int code = atoi(first.c_str());
		std::string path;
		iss >> path;
		server._error_pages[code] = path;
	} else if (directive == "client_max_body_size")
	{
		server._client_max_body_size = static_cast<size_t>(atoi(first.c_str()));
	} else if (directive == "root")
	{
		location._root = first;
	} else if (directive == "index")
	{
		location._index_file = first;
	} else if (directive == "autoindex")
	{
		location._directory_listing = (first == "on");
	} else if (directive == "cgi_path")
	{
		location._cgi_path = first;
	} else if (directive == "cgi_extension")
	{
		location._cgi_extension = first;
	} else if (directive == "upload_path")
	{
		location._upload_path = first;
	} else if (directive == "return")
	{
		location._redirect = first;
	} else if (directive == "allow")
	{
		location._allowed_methods.push_back(first);
	}
}

bool Config::checkLastChar(char c, std::string line)
{
	int i = line.length() - 1;
	while (i >= 0 && line[i] == ' ')
		i--;
	if (line[i] == c)
		return true;
	return false;
}

