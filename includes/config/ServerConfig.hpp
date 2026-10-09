#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <map>

#include "config/LocationConfig.hpp"

class ServerConfig {
private:
    std::string                         _host;
    int                                 _port;
    std::vector<std::string>            _serverNames;
    size_t                              _clientMaxBodySize;
    std::map<int, std::string>          _errorPages;
    std::vector<LocationConfig>         _locations;

public:
    ServerConfig();
    ServerConfig(const ServerConfig& other);
    ServerConfig& operator=(const ServerConfig& other);
    ~ServerConfig();

    const std::string&                  getHost() const;
    int                                 getPort() const;
    const std::vector<std::string>&     getServerNames() const;
    size_t                              getClientMaxBodySize() const;
    const std::map<int, std::string>&   getErrorPages() const;
    const std::vector<LocationConfig>&  getLocations() const;

    void setHost(const std::string& host);
    void setPort(int port);
    void setServerNames(const std::vector<std::string>& serverNames);
    void setClientMaxBodySize(size_t size);
    void setErrorPages(const std::map<int, std::string>& errorPages);
    void setLocations(const std::vector<LocationConfig>& locations);

    void addServerName(const std::string& name);
    void addErrorPage(int code, const std::string& path);
    void addLocation(const LocationConfig& location);
};

#endif