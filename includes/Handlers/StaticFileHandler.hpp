#ifndef STATICFILEHANDLER_HPP
#define STATICFILEHANDLER_HPP

#include "HttpResponse.hpp"
#include "HttpRequest.hpp"
#include "LocationConfig.hpp"

HttpResponse handleGet(const HttpRequest & request, const LocationConfig & loc);
HttpResponse handleDelete(const HttpRequest & request, const LocationConfig & loc);

#endif