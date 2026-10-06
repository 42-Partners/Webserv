#include "ServerManager/ServerManager.hpp"
#include "Connection/Connection.hpp"
#include <unistd.h>

ServerManager::ServerManager()
    : _serverConfig(),
      _listenSockets(),
      _pollFds(),
      _connections(),
      _isRunning(false) {
}

ServerManager::ServerManager(const std::vector<ServerConfig>& configs)
    : _serverConfig(configs),
      _listenSockets(),
      _pollFds(),
      _connections(),
      _isRunning(false) {
}

ServerManager::ServerManager(const ServerManager& other)
    : _serverConfig(other._serverConfig),
      _listenSockets(),
      _pollFds(),
      _connections(),
      _isRunning(other._isRunning) {
}

ServerManager& ServerManager::operator=(const ServerManager& other) {
    if (this != &other) {
        for (std::map<int, Connection*>::iterator it = _connections.begin(); it != _connections.end(); ++it) {
            if (it->first >= 0)
                close(it->first);
            delete it->second;
        }
        _connections.clear();

        for (size_t i = 0; i < _listenSockets.size(); ++i) {
            if (_listenSockets[i] >= 0)
                close(_listenSockets[i]);
        }
        _listenSockets.clear();
        _pollFds.clear();

        this->_serverConfig = other._serverConfig;
        this->_isRunning = other._isRunning;
    }
    return *this;
}

ServerManager::~ServerManager() {
    for (std::map<int, Connection*>::iterator it = _connections.begin(); it != _connections.end(); ++it) {
        if (it->first >= 0) {
            close(it->first);
        }
        delete it->second;
    }
    _connections.clear();
    for (size_t i = 0; i < _listenSockets.size(); ++i) {
        if (_listenSockets[i] >= 0) {
            close(_listenSockets[i]);
        }
    }
    _listenSockets.clear();
    _pollFds.clear();
}

void ServerManager::_setupListeningSockets()
{

}

void ServerManager::_acceptNewConnection(int listenFd)
{
    if(listenFd)
    {

    }
}

void ServerManager::_handleConnectionRead(int connectionFd, size_t pollIndex)
{
    if(connectionFd)
    {

    }
    if(pollIndex)
    {

    }
}

void ServerManager::_handleConnectionWrite(int connectionFd, size_t pollIndex)
{
    if(connectionFd)
    {

    }
    if(pollIndex)
    {

    }
}

void ServerManager::_closeConnectionConnection(int connectionFd, size_t pollIndex)
{
    if(connectionFd)
    {

    }
    if(pollIndex)
    {

    }
}

bool ServerManager::_isListeningSocket(int fd) const
{
    if(fd)
    {
        
    }
	return false;
}