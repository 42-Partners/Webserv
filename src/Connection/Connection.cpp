#include "Connection/Connection.hpp"

Connection::Connection()
	: _fd(-1),
	_serverConfig(NULL),
	_ip(),
	_readBuffer(),
	_writeBuffer(),
	_httpRequest(NULL),
	_lastActivity(time(NULL)),
	_connectionState(READING_HEADER),
	_httpResponse() {
}

Connection::Connection(int ConnectionFd, const std::string& ConnectionIp, const ServerConfig* config)
	: _fd(ConnectionFd),
	_serverConfig(config),
	_ip(ConnectionIp),
	_readBuffer(),
	_writeBuffer(),
	_httpRequest(new HttpRequest()),
	_lastActivity(time(NULL)),
	_connectionState(READING_HEADER),
	_httpResponse()
	{
	}

Connection::~Connection() {
	delete _httpRequest;
}

Connection::Connection(const Connection& other)
	: _fd(other._fd),
	_serverConfig(other._serverConfig),
	_ip(other._ip),
	_readBuffer(other._readBuffer),
	_writeBuffer(other._writeBuffer),
	_httpRequest(other._httpRequest ? new HttpRequest(*other._httpRequest) : NULL),
	_lastActivity(other._lastActivity),
	_connectionState(other._connectionState),
	_httpResponse(other._httpResponse) {
}

Connection& Connection::operator=(const Connection& other) {
	if (this != &other) {
		delete _httpRequest;
		_fd = other._fd;
		_serverConfig = other._serverConfig;
		_ip = other._ip;
		_readBuffer = other._readBuffer;
		_writeBuffer = other._writeBuffer;
		_httpRequest = (other._httpRequest ? new HttpRequest(*other._httpRequest) : NULL);
		_lastActivity = other._lastActivity;
		_connectionState = other._connectionState;
		_httpResponse = other._httpResponse;
	}
	return *this;
}

int	Connection::getFd() const {
	return this->_fd;
}

ConnectionState	Connection::getState() const {
	return this->_connectionState;
}

void	Connection::setState(ConnectionState newState) {
	this->_connectionState = newState;
}

time_t	Connection::getLastActivity() const {
	return this->_lastActivity;
}

void	Connection::updateLastActivity() {
	this->_lastActivity = time(NULL);
}

void	Connection::appendToReadBuffer(const char* buffer, size_t size) {
	if (buffer != NULL && size > 0) {
		this->_readBuffer.append(buffer, size);
	}
}

std::string&	Connection::getReadBuffer() {
	return this->_readBuffer;
}

std::string&	Connection::getWriteBuffer() {
	return this->_writeBuffer;
}

HttpRequest&	Connection::getRequest() {
	if (this->_httpRequest == NULL) {
		this->_httpRequest = new HttpRequest();
	}
	return *this->_httpRequest;
}

void Connection::resetForNextRequest() {
	if (this->_httpRequest != NULL) {
		this->_httpRequest->clear();
	}
	this->_readBuffer.clear();
	this->_writeBuffer.clear();
	this->_httpResponse.clear();
	this->_connectionState = READING_HEADER;
	this->updateLastActivity();
}

// void Connection::matchAndSetServerConfig(const std::string& hostHeader) {
// 	if (_possibleConfigs.empty()) return;
// 	for (size_t i = 0; i < _possibleConfigs.size(); ++i) {
// 		const std::vector<std::string>& serverNames = _possibleConfigs[i].getServerNames();
// 		for (size_t j = 0; j < serverNames.size(); ++j) {
// 			if (serverNames[j] == hostHeader) {
// 				_currentConfig = &_possibleConfigs[i];
// 				return;
// 			}
// 		}
// 	}
// }

const ServerConfig& Connection::getServerConfig() const
{
	return *this->_serverConfig;
}


std::string Connection::getRawResponse()
{
	std::string testResponse = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "Content-Length: 52\r\n"
    "Connection: close\r\n"
    "\r\n"
    "<html><body><h1>Funcionou! Webserv online.</h1></body></html>";
	return testResponse;
}

size_t Connection::getBytesSent() const {
		return _bytesSent;
}

void Connection::addBytesSent(size_t n) {
	_bytesSent += n;
}

void Connection::resetBytesSent() {
	_bytesSent = 0;
}