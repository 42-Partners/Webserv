#ifndef REQUESTPROCESSOR_HPP
#define REQUESTPROCESSOR_HPP

#include "Connection.hpp"
#include "ServerConfig.hpp"
#include "HttpRequest.hpp"

void process(Connection & connection, ServerConfig & config);

#endif