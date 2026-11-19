#ifndef REDIRECTHANDLER_HPP
#define REDIRECTHANDLER_HPP

#include "config/LocationConfig.hpp"
#include "HttpResponse/HttpResponse.hpp"

HttpResponse handleRedirect( const LocationConfig & );
HttpResponse makeRedirect( int status, const std::string & target );

#endif