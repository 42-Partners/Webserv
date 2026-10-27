
#include "config/ServerConfig.hpp"

// 1. Construtor Padrão (com valores default seguros)
ServerConfig::ServerConfig()
    : _host("127.0.0.1"),
      _port(8080),
      _serverNames(),
      _clientMaxBodySize(1048576), // 1 MB default
      _errorPages(),
      _locations() {
}

// 2. Construtor de Cópia
ServerConfig::ServerConfig(const ServerConfig& other)
    : _host(other._host),
      _port(other._port),
      _serverNames(other._serverNames),
      _clientMaxBodySize(other._clientMaxBodySize),
      _errorPages(other._errorPages),
      _locations(other._locations) {
}

// 3. Operador de Atribuição
ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this != &other) {
        this->_host = other._host;
        this->_port = other._port;
        this->_serverNames = other._serverNames;
        this->_clientMaxBodySize = other._clientMaxBodySize;
        this->_errorPages = other._errorPages;
        this->_locations = other._locations;
    }
    return *this;
}

// 4. Destrutor
ServerConfig::~ServerConfig() {
}

// --- Getters ---

const std::string& ServerConfig::getHost() const {
    return this->_host;
}

int ServerConfig::getPort() const {
    return this->_port;
}

const std::vector<std::string>& ServerConfig::getServerNames() const {
    return this->_serverNames;
}

size_t ServerConfig::getClientMaxBodySize() const {
    return this->_clientMaxBodySize;
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const {
    return this->_errorPages;
}

const std::vector<LocationConfig>& ServerConfig::getLocations() const {
    return this->_locations;
}

// --- Setters ---

void ServerConfig::setHost(const std::string& host) {
    this->_host = host;
}

void ServerConfig::setPort(int port) {
    this->_port = port;
}

void ServerConfig::setServerNames(const std::vector<std::string>& serverNames) {
    this->_serverNames = serverNames;
}

void ServerConfig::setClientMaxBodySize(size_t size) {
    this->_clientMaxBodySize = size;
}

void ServerConfig::setErrorPages(const std::map<int, std::string>& errorPages) {
    this->_errorPages = errorPages;
}

void ServerConfig::setLocations(const std::vector<LocationConfig>& locations) {
    this->_locations = locations;
}

// --- Métodos Utilitários ---

void ServerConfig::addServerName(const std::string& name) {
    this->_serverNames.push_back(name);
}

void ServerConfig::addErrorPage(int code, const std::string& path) {
    this->_errorPages[code] = path;
}

void ServerConfig::addLocation(const LocationConfig& location) {
    this->_locations.push_back(location);
}