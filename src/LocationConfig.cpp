/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationConfig.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ewiese-m <ewiese-m@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/11 12:47:46 by ewiese-m          #+#    #+#             */
/*   Updated: 2025/08/13 23:24:37 by ewiese-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/LocationConfig.hpp"

LocationConfig::LocationConfig() : _path(""), _root(""), _allowed_methods(),
                                   _index_file(""), _directory_listing(false), _cgi_path(""),
                                   _cgi_extension(""), _upload_path(""), _redirect("")
{
}

LocationConfig::~LocationConfig()
{
}

LocationConfig::LocationConfig(const LocationConfig &other) : _path(other._path),
                                                              _root(other._root), _allowed_methods(other._allowed_methods),
                                                              _index_file(other._index_file),
                                                              _directory_listing(other._directory_listing), _cgi_path(other._cgi_path),
                                                              _cgi_extension(other._cgi_extension), _upload_path(other._upload_path),
                                                              _redirect(other._redirect)
{
}

LocationConfig &LocationConfig::operator=(const LocationConfig &other)
{
    if (this != &other)
    {
        _path = other._path;
        _root = other._root;
        _allowed_methods = other._allowed_methods;
        _index_file = other._index_file;
        _directory_listing = other._directory_listing;
        _cgi_path = other._cgi_path;
        _cgi_extension = other._cgi_extension;
        _upload_path = other._upload_path;
        _redirect = other._redirect;
    }
    return (*this);
}
