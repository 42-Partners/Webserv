
#include "config/ConfigData.hpp"

Config::Config() {}

void Config::addServer(ServerConfig &serverConf) {
    servers.push_back(serverConf);
}

ServerConfig::ServerConfig() :
        host("0.0.0.0"),
        port(8080),
        client_max_body_size(1048576) // Nginx default: 1MB (1024 * 1024 bytes)
{}

void ServerConfig::addLocation(LocationConfig &locationConf) {
    locations.push_back(locationConf);
}

LocationConfig::LocationConfig() :
    path(""),
    root(""),
    autoindex(false),
    index("index.html"),
    cgi_extension(""),
    upload_path(""),
    redirect(std::make_pair(0, ""))
{
    this->allowed_methods.push_back("GET");
}





// ---------------------------------------------------------------------
// Print LocationConfig
// ---------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, const LocationConfig &loc) {
    os << "    [Location: " << (loc.path.empty() ? "/" : loc.path) << "]\n";
    os << "      Root         : " << loc.root << "\n";
    os << "      Autoindex    : " << (loc.autoindex ? "on" : "off") << "\n";
    os << "      Index        : " << loc.index << "\n";
    os << "      CGI Extension: " << loc.cgi_extension << "\n";
    os << "      Upload Path  : " << loc.upload_path << "\n";
    
    if (loc.redirect.first != 0) {
        os << "      Redirect     : " << loc.redirect.first << " -> " << loc.redirect.second << "\n";
    } else {
        os << "      Redirect     : none\n";
    }

    os << "      Allowed Mthds: ";
    for (size_t i = 0; i < loc.allowed_methods.size(); ++i) {
        os << loc.allowed_methods[i] << (i + 1 < loc.allowed_methods.size() ? ", " : "");
    }
    os << "\n";

    return os;
}

// ---------------------------------------------------------------------
// Print ServerConfig
// ---------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, const ServerConfig &srv) {
    os << "  ================ SERVER CONFIG ================\n";
    os << "  Host                : " << srv.host << "\n";
    os << "  Port                : " << srv.port << "\n";
    os << "  Client Max Body Size: " << srv.client_max_body_size << " bytes\n";

    os << "  Server Names        : ";
    for (size_t i = 0; i < srv.server_names.size(); ++i) {
        os << srv.server_names[i] << (i + 1 < srv.server_names.size() ? ", " : "");
    }
    os << "\n";

    os << "  Error Pages         :\n";
    if (srv.error_pages.empty()) {
        os << "    none\n";
    } else {
        for (std::map<int, std::string>::const_iterator it = srv.error_pages.begin(); it != srv.error_pages.end(); ++it) {
            os << "    " << it->first << " => " << it->second << "\n";
        }
    }

    os << "  Locations (" << srv.locations.size() << "):\n";
    for (size_t i = 0; i < srv.locations.size(); ++i) {
        os << srv.locations[i];
    }
    os << "  ===============================================\n";

    return os;
}

// ---------------------------------------------------------------------
// Print Config (Objeto principal)
// ---------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, const Config &conf) {
    os << "=================================================\n";
    os << "          WEBSERV CONFIGURATION DATA             \n";
    os << "          Total Servers: " << conf.servers.size() << "\n";
    os << "=================================================\n\n";

    for (size_t i = 0; i < conf.servers.size(); ++i) {
        os << "[Server #" << (i + 1) << "]\n";
        os << conf.servers[i] << "\n";
    }

    return os;
}