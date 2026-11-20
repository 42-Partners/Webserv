#ifndef UPLOADHANDLER_HPP
#define UPLOADHANDLER_HPP

#include "HttpResponse/HttpResponse.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "config/LocationConfig.hpp"
#include "config/ServerConfig.hpp"

HttpResponse handlePost(const HttpRequest &req, const ServerConfig & config, const LocationConfig &loc);

#endif