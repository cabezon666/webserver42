#pragma once

#include "HttpRequest.hpp"
#include "LocationConfig.hpp"
#include <string>
#include <map>
#include <vector>
#include <ctime>

class CGI {
public:
    // Constructor
    CGI(const HttpRequest& request, const LocationConfig& location);

    // Destructor
    ~CGI();

    // Main execution method
    std::string execute(const std::string& script_path);

    // Set timeout for CGI execution
    void setTimeout(time_t seconds) { timeout_seconds_ = seconds; }

private:
    // Member variables
    const HttpRequest& request_;
    const LocationConfig& location_;
    std::map<std::string, std::string> env_map_;
    std::vector<char*> env_vars_;
    time_t timeout_seconds_;

    // Setup methods
    void setupEnvironment();
    void buildEnvArray();

    // Execution methods
    void executeCGIChild(const std::string& script_path, int pipe_in[2], int pipe_out[2]);
    std::string handleCGIParent(pid_t pid, int pipe_in[2], int pipe_out[2]);

    // Parsing methods
    std::string parseCGIOutput(const std::string& raw_output);
    std::string generateErrorResponse(int code, const std::string& message);

    // Utility methods
    std::string getDirectoryPath(const std::string& file_path);
    std::string toUpperSnakeCase(const std::string& str);
    std::string toString(int num);
    std::string getStatusMessage(int code);

    // Prevent copying
    CGI(const CGI&);
    CGI& operator=(const CGI&);
};
