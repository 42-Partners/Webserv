#include "Handlers/CgiHandler.hpp"

char* createEnvString(const std::string& key, const std::string& value) {
    std::string full = key + "=" + value;
    return strdup(full.c_str());
}

std::vector<char*> buildCgiEnvironment(const HttpRequest& req, const ServerConfig& server, const LocationConfig& loc) {
    std::vector<char*> env;

    env.push_back(createEnvString("REQUEST_METHOD", req.getMethod()));
    env.push_back(createEnvString("QUERY_STRING", req.getQueryString()));
    env.push_back(createEnvString("CONTENT_TYPE", req.getHeader("Content-Type")));
    env.push_back(createEnvString("CONTENT_LENGTH", req.getHeader("Content-Length")));
    env.push_back(createEnvString("SCRIPT_NAME", req.getPath()));
    env.push_back(createEnvString("PATH_INFO", ""));
    env.push_back(createEnvString("SERVER_PROTOCOL", "HTTP/1.1"));
    env.push_back(createEnvString("SERVER_NAME", server.getServerName())); // averigua
    
    std::ostringstream portStream;
    portStream << server.getPort();
    env.push_back(createEnvString("SERVER_PORT", portStream.str()));
    
    env.push_back(createEnvString("GATEWAY_INTERFACE", "CGI/1.1"));
    env.push_back(createEnvString("REDIRECT_STATUS", "200"));

    env.push_back(NULL);
    return env;
}
