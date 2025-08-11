/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ewiese-m <ewiese-m@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/11 12:47:42 by ewiese-m          #+#    #+#             */
/*   Updated: 2025/08/11 13:35:50 by ewiese-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Config.hpp"
#include "../inc/WebServer.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        Config config(argv[1]);
        const std::vector<ServerConfig>& servers = config.getServers();

	//Iniciar servidor HTTP
	WebServer server(servers);
	server.run();

        // Print all server configurations
        for (std::vector<ServerConfig>::const_iterator s_it = servers.begin();
             s_it != servers.end(); ++s_it) {

            const ServerConfig& s = *s_it;
            std::cout << "\n=== Server Configuration ===\n";
            std::cout << "Host: " << s._host << "\n";
            std::cout << "Port: " << s._port << "\n";

            // Print server names
            if (!s._server_names.empty()) {
                std::cout << "Server Names: ";
                for (std::vector<std::string>::const_iterator sn_it = s._server_names.begin();
                     sn_it != s._server_names.end(); ++sn_it) {
                    if (sn_it != s._server_names.begin()) std::cout << ", ";
                    std::cout << *sn_it;
                }
                std::cout << "\n";
            }

            // Print error pages
            if (!s._error_pages.empty()) {
                std::cout << "Error Pages:\n";
                for (std::map<int, std::string>::const_iterator ep_it = s._error_pages.begin();
                     ep_it != s._error_pages.end(); ++ep_it) {
                    std::cout << "  " << ep_it->first << " -> " << ep_it->second << "\n";
                }
            }

            std::cout << "Client Max Body Size: " << s._client_max_body_size << "\n";

            // Print locations
            std::cout << "\nLocations:\n";
            for (std::vector<LocationConfig>::const_iterator l_it = s._locations.begin();
                 l_it != s._locations.end(); ++l_it) {

                const LocationConfig& l = *l_it;
                std::cout << "  Path: " << l._path << "\n";
                std::cout << "    Root: " << l._root << "\n";
                std::cout << "    Allow Methods: ";

                for (std::vector<std::string>::const_iterator m_it = l._allowed_methods.begin();
                     m_it != l._allowed_methods.end(); ++m_it) {
                    if (m_it != l._allowed_methods.begin()) std::cout << ", ";
                    std::cout << *m_it;
                }

                std::cout << "\n    Index: " << l._index_file
                        << "\n    Autoindex: " << (l._directory_listing ? "on" : "off")
                        << "\n    CGI Path: " << l._cgi_path
                        << "\n    CGI Ext: " << l._cgi_extension
                        << "\n    Upload: " << l._upload_path
                        << "\n    Redirect: " << l._redirect << "\n\n";
            }
            std::cout << "=============================\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Configuration Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
