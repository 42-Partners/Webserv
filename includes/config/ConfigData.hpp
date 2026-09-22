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

        LocationConfig();
};

class ServerConfig {
    public:
        std::string                         host;
        int                                 port;
        std::vector<std::string>            server_names;
        size_t                              client_max_body_size;
        std::map<int, std::string>          error_pages;
        std::vector<LocationConfig>         locations;

        void addLocation(LocationConfig &locationConf);
        ServerConfig();
};

class Config {
    public:
        std::vector<ServerConfig> servers;
        void addServer(ServerConfig &serverConf);

        Config();
};

std::ostream &operator<<(std::ostream &os, const LocationConfig &loc); // teste
std::ostream &operator<<(std::ostream &os, const ServerConfig &srv); // teste
std::ostream &operator<<(std::ostream &os, const Config &conf); // teste

#endif