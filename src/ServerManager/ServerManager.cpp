#include "ServerManager/ServerManager.hpp"

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

ServerManager::ServerManager(const std::vector<ServerConfig>& configs)
	: _serverConfigs(configs) {
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
			for (size_t i = 0; i < _pollFds.size(); ++i) {
			int fd = _pollFds[i].fd;
			short revents = _pollFds[i].revents;

			if (revents == 0) continue;

			if (_isListeningSocket(fd)) {
				if (revents & POLLIN) {
					_acceptNewConnection(fd);
				}
			} else {
				if (revents & (POLLHUP | POLLERR)) {
					std::cout << "entrou aqui POLLHUP/POLLERR" << std::endl;
					_closeConnection(fd, i);
					--i;
					continue;
				}
				if (revents & POLLIN) {
					std::cout << "entrou aqui POLLIN" << std::endl;
					_handleConnectionRead(fd, i);
				} 
				// Trata escrita
				if (revents & POLLOUT) {
					std::cout << "entrou aqui POLLOUT" << std::endl;
					_handleConnectionWrite(fd, i);
				}
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
    char buffer[1024];
    std::string accumulatedData = "";
    while (true) {
        ssize_t bytesRead = recv(connectionFd, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            _closeConnection(connectionFd, pollIndex);
            return;
        } 
        
        if (bytesRead == 0) {
            std::cout << "[ServerManager] Cliente fechou a conexão (FD: " << connectionFd << ")" << std::endl;
            _closeConnection(connectionFd, pollIndex);
            return;
        }
        buffer[bytesRead] = '\0';
        accumulatedData += buffer;
    }
    std::cout << "[ServerManager] Requisição completa lida do FD " << connectionFd << ":\n" << accumulatedData << std::endl;
    // parser para HttpRequest
	// falta desenvolver http response
	// mudança do events para encerrar requisição
	_pollFds[pollIndex].events = POLLOUT;
}

void ServerManager::_handleConnectionWrite(int connectionFd, size_t pollIndex) {
    // Stub temporário: apenas fecha a conexão para liberar o cliente (ex: o curl) 
    // e evitar que ele fique travado esperando uma resposta.
    _closeConnection(connectionFd, pollIndex);
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
// void ServerManager::_closeConnectionConnection(int connectionFd, size_t pollIndex)
// {

// }
// void ServerManager::_cleanupListeningSockets()
// {

// }

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