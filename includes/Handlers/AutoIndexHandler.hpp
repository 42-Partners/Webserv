#ifndef AUTOINDEXHANDLER_HPP
#define AUTOINDEXHANDLER_HPP

#include "config/ServerConfig.hpp"
#include "HttpResponse/HttpResponse.hpp"

#include <string>

struct fileStruct {
	std::string name;
	bool isDir;
	fileStruct() : isDir(false) {}
};

HttpResponse handleAutoindex( const ServerConfig &, const std::string &, const std::string & );

#endif