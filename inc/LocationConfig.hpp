#pragma once

#include <string>
#include <vector>

class LocationConfig {
public:
	//  --- Constructors and Destructor ---
	LocationConfig();
	~LocationConfig();
	LocationConfig(const LocationConfig& other);
	LocationConfig& operator=(const LocationConfig& other);

	// --- Public Member Variables ---
	//  All fields are public for simplicity, as this is primarily a data-structure class.

	// The URI path this location block will match (e.g., "/" or "/images/").
	std::string _path;

	// The root directory on the file system from which to serve files for this location.
	std::string _root;

	// A list of HTTP methods allowed for this location (e.g., "GET", "POST", "DELETE").
	std::vector<std::string> _allowed_methods;

	// The default file to serve when a directory is requested (e.g., "index.html").
	std::string _index_file;

	// A flag to enable or disable automatic directory listing if _index_file is not found.
	bool _directory_listing;

	// The path to the CGI executable for processing requests at this location.
	std::string _cgi_path;

	// The specific file extension that triggers the CGI script (e.g., ".php").
	std::string _cgi_extension;

	// The directory where uploaded files should be stored.
	std::string _upload_path;

	// The URL for an HTTP redirect (301 Moved Permanently).
	std::string _redirect;
};