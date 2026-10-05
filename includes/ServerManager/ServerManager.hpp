#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <string>
#include <map>
#include <vector>
#include <poll.h>

#include "config/ConfigData.hpp"
#include "Connection/Connection.hpp"

class ServerManager {
private:
	std::vector<ServerConfig>   _serverConfig;
	std::vector<int>            _listenSockets;
	std::vector<struct pollfd>  _pollFds;
	std::map<int, Connection*>  _connections;
	bool                        _isRunning;

	// Métodos auxiliares de I/O
	void _setupListeningSockets();
	void _acceptNewConnection(int listenFd);
	void _handleConnectionRead(int connectionFd, size_t pollIndex);
	void _handleConnectionWrite(int connectionFd, size_t pollIndex);
	void _closeConnection(int connectionFd, size_t pollIndex);
	void _checkTimeouts();
	bool _isListeningSocket(int fd) const;
	void _closeConnectionConnection(int connectionFd, size_t pollIndex);

	ServerManager(const ServerManager& other);
	ServerManager& operator=(const ServerManager& other);

public:
	ServerManager();
	explicit ServerManager(const std::vector<ServerConfig>& configs);
	~ServerManager();

	void run();
};

#endif