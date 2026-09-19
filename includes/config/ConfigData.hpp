#ifndef CONFIG_CONFIG_DATA_HPP
#define CONFIG_CONFIG_DATA_HPP

#include <iostream>
#include <vector>
#include <map>
#include <string>

class LocationConfig {
    public:
        std::string                 path; // ok
        std::vector<std::string>    allowed_methods; // ok
        std::string                 root; // ok
        bool                        autoindex; // ok
        std::string                 index; // ok
        std::string                 cgi_extension; // ok
        std::string                 upload_path; // ok
        std::pair<int, std::string> redirect; // ok

        LocationConfig(); // enhance with args later
};

class ServerConfig {
    public:
        std::string                         host; // ok
        int                                 port; // ok
        std::vector<std::string>            server_names; // ok
        size_t                              client_max_body_size; //ok
        std::map<int, std::string>          error_pages; // ok
        std::vector<LocationConfig>         locations;

        ServerConfig(); // enhance with args later
};

class Config {
    public:
        std::vector<ServerConfig> servers;

        Config(); // enhance with args later
};

#endif