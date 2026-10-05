#include "HttpRequest/HttpRequest.hpp"

HttpRequest::HttpRequest()
{
	
}
HttpRequest::~HttpRequest()
{
	
}

int HttpRequest::getErrorCode() const { return(this->_errorCode); }

HttpRequest::ParsingState HttpRequest::getState() const { return(this->_state); }

const std::string& HttpRequest::getMethod() const { return (this->_method); }

const std::string& HttpRequest::getUri() const { return(this->_uri); }

const std::string& HttpRequest::getPath() const { return(this->_path); }

const std::string& HttpRequest::getQueryString() const { return(this->_queryString); }

const std::string& HttpRequest::getHttpVersion() const { return(this->_httpVersion); }

const std::string& HttpRequest::getBody() const { return(this->_body); }

const std::map<std::string, std::string>& HttpRequest::getHeaders() const { return(this->_headers); }

std::string HttpRequest::getHeader(const std::string& key) const
{
	map_iterator it = this->_headers.find(key);
    if (it != this->_headers.end()) {
        return it->second;
    }
    return "";
}

bool HttpRequest::hasHeader(const std::string& key) const
{
	map_iterator it = this->_headers.find(key);
    if (it != this->_headers.end()) {
        return true;
    }
    return false;

}