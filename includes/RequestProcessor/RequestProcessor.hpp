#ifndef REQUESTPROCESSOR_HPP
#define REQUESTPROCESSOR_HPP

#include "config/ServerConfig.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"

HttpResponse process( const HttpRequest & request, const ServerConfig & config );

#endif