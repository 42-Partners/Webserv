#include "HttpResponse/HttpResponse.hpp"

HttpResponse::HttpResponse() : _statusCode(0), _closeConnection(false) { }

HttpResponse::~HttpResponse() { }

void HttpResponse::setStatus( int status ) { this->_statusCode = status; }

void HttpResponse::setHeader( const std::string& key, const std::string& value ) { this->_headers[key] = value; }

void HttpResponse::setBody( const std::string& body ) { this->_body = body; }

bool HttpResponse::isError() { return ( _statusCode >= 400 && _statusCode <= 599); }

bool HttpResponse::shouldClose() const { return this->_closeConnection; }

int HttpResponse::getStatusCode() const { return ( this->_statusCode ); }

std::string HttpResponse::getBody() const { return ( this->_body ); }

std::string HttpResponse::serialize() { return ( std::string() ); }
