#include "HttpResponse/ErrorPage.hpp"
#include "HttpResponse/HttpResponse.hpp"
#include "Utils/Utils.hpp"

#include <sstream>

HttpResponse::HttpResponse() : _statusCode(0), _keepAlive(false) { }

HttpResponse::~HttpResponse() { }

bool HttpResponse::isError() const { return _statusCode >= 400 && _statusCode <= 59; }

bool HttpResponse::getKeepAlive() const { return this->_keepAlive; }

int HttpResponse::getStatusCode() const { return this->_statusCode; }

const std::string & HttpResponse::getBody() const { return this->_body; }

void HttpResponse::setStatus( int status ) { this->_statusCode = status; }

void HttpResponse::setKeepAlive( bool state ) { this->_keepAlive = state; }

void HttpResponse::setBody( const std::string & body ) { this->_body = body; }

void HttpResponse::setHeader( const std::string & key, const std::string& value ) { this->_headers[toLower(key)] = value; }

bool HttpResponse::hasHeader( const std::string & key ) const {
	std::map<std::string, std::string>::const_iterator it = this->_headers.find( toLower(key) );

	return it != this->_headers.end();
}

std::string HttpResponse::serialize() const { 
	std::map<std::string, std::string> headers = _headers;
	
	if ( _statusCode > 199 && _statusCode != 204 &&
		_statusCode != 304 && !hasHeader( "Content-Length" ) ) {
		std::ostringstream len;
		len << _body.size();
		headers["Content-Length"] = len.str();
	}

	std::ostringstream out;
	
	out << "HTTP/1.1 " << _statusCode << " " << reasonPhrase( _statusCode ) << "\r\n";

	std::map<std::string, std::string>::const_iterator it;
	for ( it = headers.begin(); it != headers.end(); ++it )
		out << it->first << ": " << it->second << "\r\n";

	out << "\r\n";
	out << _body;

	return out.str();
}
