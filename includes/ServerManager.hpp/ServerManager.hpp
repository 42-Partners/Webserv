#include <string>
#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <stdexcept>

#include "includes/config/ConfigData.hpp"

class ServerManager {
private:
    std::vector<ServerConfig>   _serverConfig;
    std::vector<int>            _listenSockets;
    std::vector<struct pollfd>  _pollFds;
    std::map<int, Connection>   _connection;

    void _setupListeningSockets();
    void _acceptNewConnection(int listenFd);
    void _handleConnectionRead(int connectionFd, size_t pollIndex);
    void _handleConnectionWrite(int connectionFd, size_t pollIndex);
    void _closeConnectionConnection(int connectionFd, size_t pollIndex);
    bool _isListeningSocket(int fd) const;

public:
    ServerManager(const std::vector<ServerConfig>& configs);
    ~ServerManager();

    // Inicia os sockets e entra no loop do poll()
    void run();
};