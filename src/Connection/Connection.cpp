#include "Connection/Connection.hpp"
	
Connection::Connection()
{

}
Connection::Connection(int ConnectionFd, const std::string& ConnectionIp, const ServerConfig* config)
{
	
}
Connection::~Connection()
{
	
}
Connection::Connection(const Connection& other)
{
	this->_fd(other._fd),
    this->_serverConfig(other._serverConfig),
    this->_ip(other._ip),
    this->_readBuffer(other._readBuffer),
    this->_writeBuffer(other._writeBuffer),
    this->_request(other._request),
    this->_response(other._response),
    this->_lastActivity(other._lastActivity),
    this->_state(other._state)
}

int	Connection::getFd() const
{
	return this->_fd;
}
ConnectionState	Connection::getState() const
{
	return this->_state;
}
void	Connection::setState(ConnectionState newState)
{
	this->_state = newState;
}
time_t	Connection::getLastActivity() const
{
	return this-> _lastActivity;
}
void	Connection::updateLastActivity()
{

}
void	Connection::appendToReadBuffer(const char* buffer, size_t size)
{

}
std::string&	Connection::getReadBuffer()
{
	return this->_readBuffer;
}
std::string&	Connection::getWriteBuffer()
{
	return this->_writeBuffer;
}
HttpRequest&	Connection::getRequest()
{
	return this->_request;
}