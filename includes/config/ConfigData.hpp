#ifndef CONFIG_CONFIG_DATA_HPP
#define CONFIG_CONFIG_DATA_HPP

#include <iostream>
#include <vector>
#include <map>
#include <string>

class LocationConfig {
    public:
        std::string                 path;
        std::vector<std::string>    allowed_methods;
        std::string                 root;
        bool                        autoindex;
        std::string                 index;
        std::string                 cgi_extension;
        std::string                 upload_path;
        std::pair<int, std::string> redirect;

        LocationConfig(); // enhance with args later
};

class ServerConfig {
    public:
        std::string                         host;
        int                                 port;
        std::vector<std::string>            server_names;
        size_t                              client_max_body_size;
        std::map<int, std::string>          error_pages;
        std::vector<LocationConfig>         locations;

        ServerConfig(); // enhance with args later
};

class Config {
    public:
        std::vector<ServerConfig> servers;

        Config(); // enhance with args later
};

#endif