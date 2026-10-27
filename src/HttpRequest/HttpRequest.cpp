#include "HttpRequest/HttpRequest.hpp"

HttpRequest::HttpRequest()
    : _errorCode(0),
      _state(PARSING_REQUEST_LINE),
      _keepAlive(true),
      _isChunked(false),
      _hasContentLength(false),
      _contentLength(0),
      _maxBodySize(DEFAULT_MAX_BODY) {}

HttpRequest::HttpRequest( size_t maxSize ) 
    : _errorCode(0),
      _state(PARSING_REQUEST_LINE),
      _keepAlive(true),
      _isChunked(false),
      _hasContentLength(false),
      _contentLength(0),
      _maxBodySize(maxSize) {}

HttpRequest::~HttpRequest() {}

int HttpRequest::getErrorCode() const { return(this->_errorCode); }

HttpRequest::ParsingState HttpRequest::getState() const { return(this->_state); }

bool HttpRequest::getKeepAlive() const { return (this->_keepAlive); }

const std::string& HttpRequest::getMethod() const { return (this->_method); }

const std::string& HttpRequest::getUri() const { return(this->_uri); }

const std::string& HttpRequest::getPath() const { return(this->_path); }

const std::string& HttpRequest::getQueryString() const { return(this->_queryString); }

const std::string& HttpRequest::getHttpVersion() const { return(this->_httpVersion); }

const std::string& HttpRequest::getBody() const { return(this->_body); }

const std::map<std::string, std::string>& HttpRequest::getHeaders() const { return(this->_headers); }

std::string HttpRequest::getHeader(const std::string& key) const
{
	const_map_iterator it = this->_headers.find(key);

    if (it != this->_headers.end()) {
        return it->second;
    }

    return "";
}

bool HttpRequest::hasHeader(const std::string& key) const
{
	const_map_iterator it = this->_headers.find(key);

    if (it != this->_headers.end()) {
        return true;
    }

    return false;
}