#ifndef REQUESTPROCESSOR_HPP
#define REQUESTPROCESSOR_HPP

#include "Connection/Connection.hpp"
#include "config/ConfigData.hpp"
#include "HttpRequest/HttpRequest.hpp"

void process( Connection &, ServerConfig & );

#endif