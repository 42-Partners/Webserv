#ifndef REQUESTPROCESSOR_HPP
#define REQUESTPROCESSOR_HPP

#include "config/ServerConfig.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"

enum Methods {
	METHOD_GET,
	METHOD_POST,
	METHOD_DELETE,
	METHOD_UNKNOWN
};

HttpResponse process( const HttpRequest & request, const ServerConfig & config );

#endif