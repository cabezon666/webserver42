/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ewiese-m <ewiese-m@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/01 15:38:11 by ewiese-m          #+#    #+#             */
/*   Updated: 2025/08/13 23:25:06 by ewiese-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/CGI.hpp"
#include "../inc/HttpRequest.hpp"
#include "../inc/LocationConfig.hpp"
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

CGI::CGI(const HttpRequest &request,
         const LocationConfig &location) : request_(request), location_(location),
                                           env_vars_(), timeout_seconds_(30)
{
    setupEnvironment();
}

CGI::~CGI()
{
    for (size_t i = 0; i < env_vars_.size(); ++i)
    {
        delete[] env_vars_[i];
    }
}

std::string CGI::execute(const std::string &script_path)
{
    int pipe_in[2];
    int pipe_out[2];
    pid_t pid;

    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1)
    {
        return (generateErrorResponse(500, "Failed to create pipes"));
    }
    pid = fork();
    if (pid == -1)
    {
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);
        return (generateErrorResponse(500, "Failed to fork process"));
    }
    if (pid == 0)
    {
        executeCGIChild(script_path, pipe_in, pipe_out);
        exit(1);
    }
    else
    {
        return (handleCGIParent(pid, pipe_in, pipe_out));
    }
}

void CGI::setupEnvironment()
{
    size_t query_pos;

    env_map_.clear();
    env_map_["REQUEST_METHOD"] = request_.getMethod();
    env_map_["SERVER_PROTOCOL"] = request_.getHttpVersion();
    env_map_["GATEWAY_INTERFACE"] = "CGI/1.1";
    env_map_["SERVER_SOFTWARE"] = "webserv/1.0";
    env_map_["SERVER_NAME"] = "localhost";
    env_map_["SERVER_PORT"] = "8080";
    std::string uri = request_.getUri();
    query_pos = uri.find('?');
    if (query_pos != std::string::npos)
    {
        env_map_["PATH_INFO"] = uri.substr(0, query_pos);
        env_map_["QUERY_STRING"] = uri.substr(query_pos + 1);
    }
    else
    {
        env_map_["PATH_INFO"] = uri;
        env_map_["QUERY_STRING"] = "";
    }
    const std::map<std::string, std::string> &headers = request_.getHeaders();
    for (std::map<std::string,
                  std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it)
    {
        std::string env_name = "HTTP_" + toUpperSnakeCase(it->first);
        env_map_[env_name] = it->second;
    }
    if (request_.getMethod() == "POST")
    {
        std::string content_type = request_.getHeader("content-type");
        std::string content_length = request_.getHeader("content-length");
        if (!content_type.empty())
        {
            env_map_["CONTENT_TYPE"] = content_type;
        }
        if (!content_length.empty())
        {
            env_map_["CONTENT_LENGTH"] = content_length;
        }
    }
    buildEnvArray();
}

void CGI::buildEnvArray()
{
    char *env_var;

    for (size_t i = 0; i < env_vars_.size(); ++i)
    {
        delete[] env_vars_[i];
    }
    env_vars_.clear();
    for (std::map<std::string,
                  std::string>::const_iterator it = env_map_.begin();
         it != env_map_.end(); ++it)
    {
        std::string env_str = it->first + "=" + it->second;
        env_var = new char[env_str.length() + 1];
        std::strcpy(env_var, env_str.c_str());
        env_vars_.push_back(env_var);
    }
    env_vars_.push_back(NULL);
}

void CGI::executeCGIChild(const std::string &script_path, int pipe_in[2],
                          int pipe_out[2])
{
    const char *argv[3];

    close(pipe_in[1]);
    dup2(pipe_in[0], STDIN_FILENO);
    close(pipe_in[0]);
    close(pipe_out[0]);
    dup2(pipe_out[1], STDOUT_FILENO);
    close(pipe_out[1]);
    std::string directory = getDirectoryPath(script_path);
    if (!directory.empty())
    {
        chdir(directory.c_str());
    }
    argv[0] = location_._cgi_path.c_str();
    argv[1] = script_path.c_str();
    argv[2] = NULL;
    execve(argv[0], const_cast<char **>(argv), &env_vars_[0]);
    std::cerr << "CGI execution failed: " << std::endl;
    exit(1);
}

std::string CGI::handleCGIParent(pid_t pid, int pipe_in[2], int pipe_out[2])
{
    int flags;
    char buffer[4096];
    time_t start_time;
    int status;
    pid_t result;
    ssize_t bytes;

    close(pipe_in[0]);
    close(pipe_out[1]);
    if (request_.getMethod() == "POST" && !request_.getBody().empty())
    {
        write(pipe_in[1], request_.getBody().c_str(),
              request_.getBody().length());
    }
    close(pipe_in[1]);
    flags = fcntl(pipe_out[0], F_GETFL, 0);
    fcntl(pipe_out[0], F_SETFL, flags | O_NONBLOCK);
    std::string output;
    start_time = time(NULL);
    while (true)
    {
        if (time(NULL) - start_time > timeout_seconds_)
        {
            kill(pid, SIGTERM);
            waitpid(pid, &status, 0);
            close(pipe_out[0]);
            return (generateErrorResponse(504, "CGI timeout"));
        }
        result = waitpid(pid, &status, WNOHANG);
        if (result == pid)
        {
            while ((bytes = read(pipe_out[0], buffer, sizeof(buffer))) > 0)
            {
                output.append(buffer, bytes);
            }
            break;
        }
        bytes = read(pipe_out[0], buffer, sizeof(buffer));
        if (bytes > 0)
        {
            output.append(buffer, bytes);
        }
        else if (bytes == 0)
        {
            break;
        }
        usleep(10000);
    }
    close(pipe_out[0]);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
    {
        return (generateErrorResponse(500, "CGI script error"));
    }
    return (parseCGIOutput(output));
}

std::string CGI::parseCGIOutput(const std::string &raw_output)
{
    size_t separator;
    size_t status_pos;
    size_t status_end;

    if (raw_output.empty())
    {
        return (generateErrorResponse(500, "Empty CGI response"));
    }
    separator = raw_output.find("\r\n\r\n");
    if (separator == std::string::npos)
    {
        separator = raw_output.find("\n\n");
        if (separator == std::string::npos)
        {
            return "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: " +
                   toString(raw_output.length()) +
                   "\r\n"
                   "Connection: close\r\n"
                   "\r\n" +
                   raw_output;
        }
    }
    std::string headers = raw_output.substr(0, separator);
    std::string body = raw_output.substr(separator + (raw_output[separator] == '\r' ? 4 : 2));
    std::ostringstream response;
    if (headers.find("Status:") != std::string::npos)
    {
        status_pos = headers.find("Status:");
        status_end = headers.find('\n', status_pos);
        std::string status_line = headers.substr(status_pos + 7, status_end - status_pos - 7);
        response << "HTTP/1.1 " << status_line << "\r\n";
    }
    else
    {
        response << "HTTP/1.1 200 OK\r\n";
    }
    response << headers;
    if (headers[headers.length() - 1] != '\n')
    {
        response << "\r\n";
    }
    if (headers.find("Content-Length:") == std::string::npos && headers.find("content-length:") == std::string::npos)
    {
        response << "Content-Length: " << body.length() << "\r\n";
    }
    response << "Server: webserv/1.0\r\n";
    response << "\r\n";
    response << body;
    return (response.str());
}

std::string CGI::generateErrorResponse(int code, const std::string &message)
{
    std::ostringstream response;
    std::string body = "<!DOCTYPE html>\n"
                       "<html>\n"
                       "<head><title>CGI Error</title></head>\n"
                       "<body>\n"
                       "<h1>Error " +
                       toString(code) +
                       "</h1>\n"
                       "<p>" +
                       message +
                       "</p>\n"
                       "</body>\n"
                       "</html>\n";
    response << "HTTP/1.1 " << code << " " << getStatusMessage(code) << "\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    return (response.str());
}

std::string CGI::getDirectoryPath(const std::string &file_path)
{
    size_t last_slash;

    last_slash = file_path.find_last_of('/');
    if (last_slash != std::string::npos)
    {
        return (file_path.substr(0, last_slash));
    }
    return ("");
}

std::string CGI::toUpperSnakeCase(const std::string &str)
{
    std::string result;
    for (size_t i = 0; i < str.length(); ++i)
    {
        if (str[i] == '-')
        {
            result += '_';
        }
        else
        {
            result += std::toupper(str[i]);
        }
    }
    return (result);
}

std::string CGI::toString(int num)
{
    std::ostringstream oss;
    oss << num;
    return (oss.str());
}

std::string CGI::getStatusMessage(int code)
{
    switch (code)
    {
    case 200:
        return ("OK");
    case 400:
        return ("Bad Request");
    case 404:
        return ("Not Found");
    case 500:
        return ("Internal Server Error");
    case 504:
        return ("Gateway Timeout");
    default:
        return ("Error");
    }
}
