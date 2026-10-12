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
			int flags = fcntl(listenFd, F_GETFL, 0);
			if (flags == -1) {
				throw std::runtime_error("Fail: error to get flag F_GETFL in file socket.");
			}
			flags |= O_NONBLOCK;
			if (fcntl(listenFd, F_SETFL, flags) == -1) {
				throw std::runtime_error("Fail: error to define socket how non block.");
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

			std::cout << "[ServerManager] Socket em LISTEN: " 
					<< host << ":" << port << " (FD: " << listenFd << ")" << std::endl;

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
	std::cout << "[INFO] Pressione ENTER no terminal para encerrar o teste da Tarefa 6..." << std::endl;

	std::cin.get();
}