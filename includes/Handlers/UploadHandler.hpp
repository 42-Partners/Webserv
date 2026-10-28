#ifndef UPLOADHANDLER_HPP
#define UPLOADHANDLER_HPP

#include "HttpResponse/HttpResponse.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "config/LocationConfig.hpp"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

HttpResponse handlePost(const HttpRequest &req, const LocationConfig &loc);

bool parseMultipart(const std::string& body, const std::string& boundary, const std::string& uploadStore);

#endif