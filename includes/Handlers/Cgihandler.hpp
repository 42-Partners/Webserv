#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "config/LocationConfig.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"
#include "Utils/Utils.hpp"
#include <vector>
#include <sstream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <iostream>
#include <cstdlib>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>

struct CgiSession {
    int pid;
    int pipe_in[2];
    int pipe_out[2];
    
    std::string requestBody;
    size_t bytesWritten;
    
    std::string cgiOutput;
    bool isFinished;
};

std::vector<char*> buildCgiEnvironment(const HttpRequest& req, int serverPort, const std::string& serverName);

HttpResponse executeCgi(
    const HttpRequest& req,
    const std::string& scriptPath,
    int serverPort,
    const std::string& serverName);

#endif