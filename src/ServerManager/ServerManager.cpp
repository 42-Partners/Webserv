#include "ServerManager/ServerManager.hpp"
#include "RequestProcessor/RequestProcessor.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <set>
#include <utility>
#include <stdexcept>
#include <sys/poll.h>
#include <errno.h>
#include <algorithm>
#include <signal.h>
#include "Utils/Utils.hpp"

static void prepareRequest(Connection* conn, const std::vector<ServerConfig>& configs) {
	const ServerConfig& activeConfig = conn->getServerConfig();
	const ServerConfig* defaultConfig = &activeConfig;
	for (size_t i = 0; i < configs.size(); ++i) {
		if (configs[i].getHost() == activeConfig.getHost()
			&& configs[i].getPort() == activeConfig.getPort()) {
			defaultConfig = &configs[i];
			break;
		}
	}

	const ServerConfig* selectedConfig = defaultConfig;
	const std::string& buffer = conn->getReadBuffer();
	size_t lineEnd = buffer.find("\r\n");
	if (lineEnd != std::string::npos) {
		size_t headersEnd = buffer.find("\r\n\r\n", lineEnd);
		size_t limit = headersEnd == std::string::npos ? buffer.size() : headersEnd;
		size_t lineStart = lineEnd + 2;
		while (lineStart < limit) {
			lineEnd = buffer.find("\r\n", lineStart);
			if (lineEnd == std::string::npos || lineEnd > limit)
				break;

			std::string line = buffer.substr(lineStart, lineEnd - lineStart);
			size_t colon = line.find(':');
			if (colon != std::string::npos
				&& toLower(trim(line.substr(0, colon))) == "host") {
				std::string host = toLower(trim(line.substr(colon + 1)));
				if (!host.empty() && host[0] != '[') {
					size_t portSeparator = host.find(':');
					if (portSeparator != std::string::npos)
						host.erase(portSeparator);
				}

				for (size_t i = 0; i < configs.size(); ++i) {
					if (configs[i].getHost() != defaultConfig->getHost()
						|| configs[i].getPort() != defaultConfig->getPort())
						continue;

					const std::vector<std::string>& names = configs[i].getServerNames();
					for (size_t j = 0; j < names.size(); ++j) {
						if (toLower(names[j]) == host) {
							selectedConfig = &configs[i];
							break;
						}
					}
					if (selectedConfig != defaultConfig)
						break;
				}
				break;
			}
			lineStart = lineEnd + 2;
		}
	}

	conn->setServerConfig(selectedConfig);
	conn->getRequest().setMaxBodySize(selectedConfig->getClientMaxBodySize());
}

static void queueResponse(Connection* conn, bool peerClosed) {
	HttpRequest& request = conn->getRequest();
	HttpResponse response = process(request, conn->getServerConfig());
	bool keepAlive = !peerClosed
		&& request.getState() == HttpRequest::REQUEST_COMPLETE
		&& request.getKeepAlive()
		&& response.getStatusCode() < 500;

	response.setKeepAlive(keepAlive);
	response.setHeader("Connection", keepAlive ? "keep-alive" : "close");
	conn->setHttpResponse(response);
}

ServerManager::ServerManager(const std::vector<ServerConfig>& configs)
	: _serverConfigs(configs) {
	signal(SIGPIPE, SIG_IGN);
}

ServerManager::~ServerManager() {
	_cleanupListeningSockets();
}

void ServerManager::_cleanupListeningSockets() {
	for (size_t i = 0; i < _listenSockets.size(); ++i) {
		if (_listenSockets[i] >= 0) {
			close(_listenSockets[i]);
		}
	}
	_listenSockets.clear();
}

void ServerManager::_setupListeningSockets() {
	std::set<std::pair<std::string, int> > boundAddresses;

	for (size_t i = 0; i < _serverConfigs.size(); ++i) {
		std::string host = _serverConfigs[i].getHost();
		int port = _serverConfigs[i].getPort();
		std::pair<std::string, int> addrPair(host, port);
		if (boundAddresses.find(addrPair) != boundAddresses.end()) {
			continue;
		}

		int listenFd = -1;

		try {
			listenFd = socket(AF_INET, SOCK_STREAM, 0);
			if (listenFd < 0) {
				throw std::runtime_error("Falha ao criar o socket.");
			}
			if (fcntl(listenFd, F_SETFL, O_NONBLOCK) == -1) {
				throw std::runtime_error("Fail: error to define socket as non block.");
			}
			int opt = 1;
			if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
				throw std::runtime_error("Falha no setsockopt(SO_REUSEADDR).");
			}
			struct sockaddr_in address;
			//struct sockaddr_in {
			//	short          sin_family;  // Família de endereços (Sempre AF_INET para IPv4)
			//	unsigned short sin_port;    // Número da porta (em Network Byte Order)
			//	struct in_addr sin_addr;    // Estrutura para o endereço IP
			//	char           sin_zero[8]; // Preenchimento para alinhar o tamanho (deve ser zerado)
			//};
			// limpar a estrutura boa pratica
			std::memset(&address, 0, sizeof(address));
			address.sin_family = AF_INET;
			address.sin_port = htons(port);

			in_addr_t ip = inet_addr(host.c_str());
			if (ip == INADDR_NONE && host != "255.255.255.255") {
				throw std::runtime_error("Endereco IP invalido: " + host);
			}
			address.sin_addr.s_addr = ip;
			if (bind(listenFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
				std::stringstream ss;
				ss << "Falha no bind() no endereco " << host << ":" << port;
				throw std::runtime_error(ss.str());
			}
			if (listen(listenFd, SOMAXCONN) < 0) {
				throw std::runtime_error("Falha no listen().");
			}
			_listenSockets.push_back(listenFd);
			boundAddresses.insert(addrPair);
			_listenSocketMap[listenFd] = &_serverConfigs[i];
			// std::cout << "[ServerManager] Socket em LISTEN: " 
			// 		<< host << ":" << port << " (FD: " << listenFd << ")" << std::endl;

		} catch (const std::exception& e) {
			if (listenFd >= 0) {
				close(listenFd);
			}
			_cleanupListeningSockets();
			throw;
		}
	}
}

void ServerManager::run() {
	_setupListeningSockets();

	std::cout << "\n[OK] Todas as portas foram abertas em LISTEN." << std::endl;
	struct pollfd structPollFds;
	for (size_t i = 0; i < _listenSockets.size(); ++i) {
		structPollFds.fd = _listenSockets[i];
		structPollFds.events = POLLIN;
		structPollFds.revents = 0;
		_pollFds.push_back(structPollFds);
	}
	_isRunning = true;
	std::time_t lastTimeoutCheck = std::time(NULL);
	try{

		while (_isRunning) {
			int pollCount = poll(&_pollFds[0], _pollFds.size(), 1000);

			if (pollCount < 0) {
				std::cout << "[DEBUG] poll falhou. errno = " << errno << " (EINTR é " << EINTR << ")" << std::endl;
				if (errno == EINTR) continue;
				throw std::runtime_error("Erro na função poll()\n");
			}
			std::time_t now = std::time(NULL);
			if (now - lastTimeoutCheck >= 1) {
				_checkTimeouts();
				lastTimeoutCheck = now;
			}

			if (pollCount == 0) {
				continue;
			}
			for (size_t i = 0; i < _pollFds.size();) {
				int fd = _pollFds[i].fd;
				short revents = _pollFds[i].revents;

				if (revents == 0) {
					++i;
					continue;
				}

				if (_isListeningSocket(fd)) {
					if (revents & POLLIN) {
						_acceptNewConnection(fd);
					}
					++i;
				} else {
					if (revents & POLLERR) {
						_closeConnection(fd, i);
						continue;
					}

					std::map<int, Connection*>::iterator connection = _connections.find(fd);
					if (connection == _connections.end()) {
						_pollFds.erase(_pollFds.begin() + i);
						continue;
					}

					if (revents & (POLLIN | POLLHUP)) {
						_handleConnectionRead(fd, i);
					} 

					connection = _connections.find(fd);
					if (connection == _connections.end())
						continue;

					bool responsePending =
						connection->second->getState() == KEEP_ALIVE_WAIT
						|| connection->second->getState() == READY_TO_CLOSE;
					if (responsePending && ((revents & POLLOUT) || (revents & POLLHUP))) {
						_handleConnectionWrite(fd, i);
					}
					if (_connections.find(fd) != _connections.end())
						++i;
				}
			}
		}

	} catch (const std::exception& e) {
		_cleanupListeningSockets();
		throw;
	}
}

void ServerManager::_acceptNewConnection(int listenFd)
{
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	int clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &clientAddrLen);
	if (clientFd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return;
		}
		std::cerr << "[Erro] Falha ao executar accept()" << std::endl;
		return;
	}
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1) {
		std::cerr << "[Erro] Falha ao aplicar O_NONBLOCK no cliente FD: " << clientFd << std::endl;
		close(clientFd);
		return;
	}
	char ipBuffer[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(clientAddr.sin_addr), ipBuffer, INET_ADDRSTRLEN);
	std::string clientIp(ipBuffer);
	const ServerConfig* config = _getConfigForListenSocket(listenFd);
	_connections[clientFd] = new Connection(clientFd, clientIp, config);
	struct pollfd clientPollFd;
	clientPollFd.fd = clientFd;
	clientPollFd.events = POLLIN;
	clientPollFd.revents = 0;
	_pollFds.push_back(clientPollFd);
	std::cout << "[ServerManager] Novo cliente conectado no FD: " << clientFd << std::endl;
}

void ServerManager::_checkTimeouts() {
	std::time_t now = std::time(NULL);
	long timeoutLimit = 60;

	std::map<int, Connection*>::iterator it = _connections.begin();
	while (it != _connections.end()) {
		int fd = it->first;
		Connection* conn = it->second;
		if (now - conn->getLastActivity() > timeoutLimit) {
			std::cout << "[ServerManager] Timeout atingido para o cliente FD: " << fd << ". Fechando conexão." << std::endl;
			size_t pollIndex = _findPollIndex(fd);
			std::map<int, Connection*>::iterator toDelete = it;
			++it;
			if (pollIndex != (size_t)-1) {
				_closeConnection(fd, pollIndex);
			} else {
				delete toDelete->second;
				_connections.erase(toDelete);
				close(fd);
			}
		} else {
			++it;
		}
	}
}

size_t ServerManager::_findPollIndex(int fd) const {
	for (size_t i = 0; i < _pollFds.size(); ++i) {
		if (_pollFds[i].fd == fd) {
			return i;
		}
	}
	return (size_t)-1;
}

void ServerManager::_handleConnectionRead(int connectionFd, size_t pollIndex) {
	char buffer[4096];
	std::string accumulatedData;
	bool peerClosed = false;

	while (true) {
		ssize_t bytesRead = recv(connectionFd, buffer, sizeof(buffer), 0);

		if (bytesRead < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			}
			if (errno == EINTR)
				continue;
			_closeConnection(connectionFd, pollIndex);
			return;
		} 
		
		if (bytesRead == 0) {
			peerClosed = true;
			break;
		}

		accumulatedData.append(buffer, static_cast<size_t>(bytesRead));
		std::map<int, Connection*>::iterator active = _connections.find(connectionFd);
		if (active == _connections.end())
			return;
		active->second->updateLastActivity();
	}
	std::map<int, Connection*>::iterator it = _connections.find(connectionFd);
	if (it == _connections.end())
		return;
	Connection* conn = it->second;

	if (conn->getState() == KEEP_ALIVE_WAIT
		|| conn->getState() == READY_TO_CLOSE) {
		conn->getReadBuffer().append(accumulatedData);
		if (peerClosed && conn->getState() == KEEP_ALIVE_WAIT)
			conn->setState(READY_TO_CLOSE);
		return;
	}

	conn->getReadBuffer().append(accumulatedData);
	prepareRequest(conn, _serverConfigs);
	conn->getRequest().feed(conn->getReadBuffer());

	HttpRequest::ParsingState state = conn->getRequest().getState();
	if (state == HttpRequest::REQUEST_COMPLETE || state == HttpRequest::REQUEST_ERROR) {
		queueResponse(conn, peerClosed);
		_pollFds[pollIndex].events = POLLOUT;
	} else if (peerClosed) {
		_closeConnection(connectionFd, pollIndex);
	}
}


void ServerManager::_handleConnectionWrite(int connectionFd, size_t pollIndex) {
    Connection* conn = _connections[connectionFd];
    if (!conn) return;

    const std::string responseData = conn->getRawResponse();
    size_t totalToSend = responseData.size();
    size_t alreadySent = conn->getBytesSent(); 
    ssize_t bytesSent = send(connectionFd, responseData.c_str() + alreadySent, totalToSend - alreadySent, 0);

    if (bytesSent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        _closeConnection(connectionFd, pollIndex);
        return;
    }
	if (bytesSent == 0) {
		_closeConnection(connectionFd, pollIndex);
		return;
	}
    conn->addBytesSent(static_cast<size_t>(bytesSent));
	conn->updateLastActivity();
    if (conn->getBytesSent() >= totalToSend) {
		if (conn->getState() == READY_TO_CLOSE) {
			_closeConnection(connectionFd, pollIndex);
			return;
		}

		conn->resetForNextRequest();
		prepareRequest(conn, _serverConfigs);
		conn->getRequest().feed(conn->getReadBuffer());
		HttpRequest::ParsingState state = conn->getRequest().getState();
		if (state == HttpRequest::REQUEST_COMPLETE || state == HttpRequest::REQUEST_ERROR) {
			queueResponse(conn, false);
			_pollFds[pollIndex].events = POLLOUT;
		} else {
			_pollFds[pollIndex].events = POLLIN;
		}
    }
}


void ServerManager::_closeConnection(int connectionFd, size_t pollIndex) {
	std::cout << "[ServerManager] Fechando conexão do FD: " << connectionFd << std::endl;
	close(connectionFd);
	std::map<int, Connection*>::iterator it = _connections.find(connectionFd);
	if (it != _connections.end()) {
		delete it->second;
		_connections.erase(it);
	}
	if (pollIndex < _pollFds.size()) {
		_pollFds.erase(_pollFds.begin() + pollIndex);
	}
}

bool ServerManager::_isListeningSocket(int fd) const {
	for (size_t i = 0; i < _listenSockets.size(); ++i) {
		if (_listenSockets[i] == fd) {
			return true;
		}
	}
	return false;
}

const ServerConfig* ServerManager::_getConfigForListenSocket(int listenFd) const {
	std::map<int, const ServerConfig*>::const_iterator it = _listenSocketMap.find(listenFd);
	
	if (it != _listenSocketMap.end()) {
		return it->second;
	}
	if (!_serverConfigs.empty()) {
		return &_serverConfigs[0];
	}
	return NULL;
}