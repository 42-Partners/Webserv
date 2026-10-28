#ifndef REQUESTPROCESSOR_HPP
#define REQUESTPROCESSOR_HPP

#include "Connection/Connection.hpp"
#include "config/ServerConfig.hpp"
#include "HttpRequest/HttpRequest.hpp"

void process(Connection & connection, ServerConfig & config);

#endif