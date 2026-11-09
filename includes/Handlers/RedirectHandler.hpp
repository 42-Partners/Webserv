#ifndef REDIRECTHANDLER_HPP
#define REDIRECTHANDLER_HPP

#include "config/LocationConfig.hpp"
#include "HttpResponse/HttpResponse.hpp"

HttpResponse handleRedirect( const LocationConfig & );

#endif