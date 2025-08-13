#pragma once

#include "ServerConfig.hpp"
#include <vector>
#include <string>

class Config {
public:
    Config(const std::string& file_path);
    ~Config();

    const std::vector<ServerConfig>& getServers() const;
    const ServerConfig& findServerConfigForRequest(const std::string& host, int port) const;

    void parse(const std::string& file_path);
    static bool checkLastChar(char c, std::string line);

private:
    std::vector<ServerConfig> _servers;

    // Disable copy constructor and assignment operator
    Config();
    Config(const Config& other);
    Config& operator=(const Config& other);

    // Parsing methods
    void parseServerBlock(std::istream& file, bool *closed);
    void parseLocationBlock(std::istream& file, ServerConfig& server, bool *closed);
    void parseDirective(ServerConfig &server, LocationConfig &location,
                       const std::string &directive, const std::string &value);

    // New parsing methods for the improved parser
    void parseServerDirective(ServerConfig &server, const std::string &directive,
                             const std::string &value);
    void parseLocationDirective(LocationConfig &location, const std::string &directive,
                               const std::string &value);
};
