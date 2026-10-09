//não sei pq, mas não veio as funçoes do arquivo populei de forma generica para não dar erro ao compilar

#include "HttpResponse/HttpResponse.hpp"

HttpResponse::HttpResponse()
	: _statusCode(0)
{
}

HttpResponse::HttpResponse(const HttpResponse& src)
	: _statusCode(src._statusCode),
	  _headers(src._headers),
	  _body(src._body)
{
}

HttpResponse& HttpResponse::operator=(const HttpResponse& src)
{
	if (this != &src)
	{
		this->_statusCode = src._statusCode;
		this->_headers = src._headers;
		this->_body = src._body;
	}
	return (*this);
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::setStatus(int status)
{
	this->_statusCode = status;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value)
{
	this->_headers[key] = value;
}

void HttpResponse::setBody(const std::string& body)
{
	this->_body = body;
}

int HttpResponse::getStatusCode() const
{
	return (this->_statusCode);
}

std::string HttpResponse::serialize()
{
	return (std::string());
}

void HttpResponse::clear()
{
	this->_statusCode = 0;
	this->_headers.clear();
	this->_body.clear();
}
