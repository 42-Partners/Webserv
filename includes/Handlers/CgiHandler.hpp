#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "config/LocationConfig.hpp"
#include "config/ServerConfig.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"
#include "Utils/Utils.hpp"
#include "StatusCode/StatusCode.hpp"

#include <iostream>
#include <sstream>
#include <string>

struct CgiSession {
    int pid;
    int pipe_in[2];
    int pipe_out[2];
    
    std::string requestBody;
    size_t bytesWritten;
    
    std::string cgiOutput;
    bool isFinished;
};

HttpResponse handleCgi(const HttpRequest & request, const ServerConfig & config, const LocationConfig & loc);

#endif