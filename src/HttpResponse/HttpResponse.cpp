#include "HttpResponse/HttpResponse.hpp"

HttpResponse::HttpResponse() : _statusCode(0) { }

HttpResponse::~HttpResponse() { }

bool HttpResponse::isError() { return ( _statusCode >= 400 && _statusCode <= 599); }

void HttpResponse::setStatus( int status ) { this->_statusCode = status; }

void HttpResponse::setHeader( const std::string& key, const std::string& value ) { this->_headers[key] = value; }

void HttpResponse::setBody( const std::string& body ) { this->_body = body; }

int HttpResponse::getStatusCode() const { return ( this->_statusCode ); }

std::string HttpResponse::getBody() const { return ( this->_body ); }


std::string HttpResponse::serialize()
{
	return ( std::string() );
}

void HttpResponse::clear()
{
	this->_statusCode = 0;
	this->_headers.clear();
	this->_body.clear();
}
