#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "config/LocationConfig.hpp"
#include "request/HttpRequest.hpp"
#include <vector>

HttpResponse executeCgi(const HttpRequest& req, const LocationConfig& loc);

std::vector<char*> buildCgiEnv(const HttpRequest& req, const ServerConfig& server, const LocationConfig& loc);

string forkAndRunCgi(const std::string& script, const std::vector<char*>& env);

#endif