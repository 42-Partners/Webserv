#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include <ctime>

#include "config/ConfigData.hpp"
#include "HttpRequest/HttpRequest.hpp"

enum ConnectionState {
    READING_HEADER,
    READING_BODY,
    PROCESSING,
    WRITING_RESPONSE,
    KEEP_ALIVE_WAIT,
    READY_TO_CLOSE
};

class Connection {
private:
    int                 _fd;
    const ServerConfig* _serverConfig;
    std::string         _ip;
    std::string         _readBuffer;
    std::string         _writeBuffer;
    HttpRequest         _httpRequest;
    time_t              _lastActivity;
    ConnectionState     _connectionState;

public:
    Connection();
    Connection(int ConnectionFd, const std::string& ConnectionIp, const ServerConfig* config);
    ~Connection();
    int                 getFd() const;
    ConnectionState     getState() const;
    void                setState(ConnectionState newState);
    time_t              getLastActivity() const;
    void                updateLastActivity();
    void                appendToReadBuffer(const char* buffer, size_t size);
    std::string&        getReadBuffer();
    std::string&        getWriteBuffer();
    HttpRequest&        getRequest();


    // Esta função pertence ao objeto que gerencia a conexão do cliente. Ela orquestra o reset de 
    // todos os componentes da conexão para permitir que o mesmo socket TCP leia uma nova requisição vinda 
    // do navegador sem precisar fechar a porta (accept/close).
    // Ela chama o _request.clear() internamente, além de resetar os outros módulos da conexão:
    void                resetForNextRequest();
};

#endif