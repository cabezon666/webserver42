/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ewiese-m <ewiese-m@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/01 15:37:41 by ewiese-m          #+#    #+#             */
/*   Updated: 2025/08/13 22:40:26 by ewiese-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <ctime>
#include <string>
#include <vector>

bool	isDirectory(const std::string &path);
bool	isFile(const std::string &path);
bool	fileExists(const std::string &path);
bool	isReadable(const std::string &path);
bool	isWritable(const std::string &path);
bool	isExecutable(const std::string &path);
size_t	getFileSize(const std::string &path);
std::string readFile(const std::string &path);
bool	writeFile(const std::string &path, const std::string &content);
std::string generateDirectoryListing(const std::string &path,
	const std::string &uri);
std::string formatFileSize(size_t size);
std::string formatTime(time_t timestamp);
std::string getMimeType(const std::string &path);
std::string urlDecode(const std::string &str);
std::string urlEncode(const std::string &str);
std::string trim(const std::string &str);
std::string toLowerCase(const std::string &str);
std::string toUpperCase(const std::string &str);
std::vector<std::string> split(const std::string &str, char delimiter);
std::string join(const std::vector<std::string> &strings,
	const std::string &delimiter);
