#ifndef STATICFILEHANDLER_HPP
#define STATICFILEHANDLER_HPP

#include "config/LocationConfig.hpp"
#include "config/ServerConfig.hpp"
#include "HttpResponse/HttpResponse.hpp"
#include "HttpRequest/HttpRequest.hpp"

std::string getMimeType(const std::string& path);
HttpResponse handleGet(const HttpRequest & request, const ServerConfig & config, const LocationConfig & loc);
HttpResponse handleDelete(const HttpRequest & request, const ServerConfig & config, const LocationConfig & loc);

#endif