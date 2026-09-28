#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <ctime>

#include "config/ConfigData.hpp"
#include "HttpRequest/HttpRequest.hpp"

enum ClientState {
    READING_HEADER,
    READING_BODY,
    PROCESSING,
    WRITING_RESPONSE,
    KEEP_ALIVE_WAIT,
    READY_TO_CLOSE
};

class Client {
private:
    int                 _fd;
    const ServerConfig* _serverConfig;
    std::string         _ip;
    std::string         _readBuffer;
    std::string         _writeBuffer;
    HttpRequest         _request;
    time_t              _lastActivity;
    ClientState         _state;

public:
    Client();
    
    Client(int clientFd, const std::string& clientIp, const ServerConfig* config);
    
    ~Client();

    int                 getFd() const;
    ClientState         getState() const;
    void                setState(ClientState newState);
    time_t              getLastActivity() const;
    
    void                updateLastActivity();
    void                appendToReadBuffer(const char* buffer, size_t size);
    std::string&        getReadBuffer();
    std::string&        getWriteBuffer();
    HttpRequest&        getRequest();
    
    void                resetForNextRequest();
};

#endif