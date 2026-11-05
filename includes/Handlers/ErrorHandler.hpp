#ifndef ERRORHANDLER_HPP
#define ERRORHANDLER_HPP

#include "config/ServerConfig.hpp"
#include "HttpResponse/HttpResponse.hpp"

HttpResponse handleError( const ServerConfig &, int );

#endif