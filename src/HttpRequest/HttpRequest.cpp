#include "HttpRequest/HttpRequest.hpp"

HttpRequest::HttpRequest() : _errorCode(0), _state(PARSING_REQUEST_LINE), _isChunked(false), _hasContentLength(false), _contentLength(0) { }

HttpRequest::~HttpRequest() { }

int HttpRequest::getErrorCode() const { return(this->_errorCode); }

HttpRequest::ParsingState HttpRequest::getState() const { return(this->_state); }

void HttpRequest::setMethod(const std::string& method) { this->_method = method; }

void HttpRequest::setUri(const std::string& uri) { this->_uri = uri; }

void HttpRequest::setPath(const std::string& path) { this->_path = path; }

void HttpRequest::setQueryString(const std::string& query) { this->_queryString = query; }

void HttpRequest::setHttpVersion(const std::string& version) { this->_httpVersion = version; }

void HttpRequest::addHeader(const std::string& key, const std::string& value) { this->_headers[key] = value; }

void HttpRequest::setBody(const std::string& body) { this->_body = body; }

void HttpRequest::appendBody(const std::string& chunk) { this->_body.append(chunk); }

void HttpRequest::setState(ParsingState state) { this->_state = state; }

void HttpRequest::setErrorCode(int code) { this->_errorCode = code; }

void HttpRequest::setContentLength(size_t length) { this->_contentLength = length; }

void HttpRequest::setIsChunked(bool isChunked) { this->_isChunked = isChunked; }

const std::string& HttpRequest::getMethod() const { return (this->_method); }

const std::string& HttpRequest::getUri() const { return(this->_uri); }

const std::string& HttpRequest::getPath() const { return(this->_path); }

const std::string& HttpRequest::getQueryString() const { return(this->_queryString); }

const std::string& HttpRequest::getHttpVersion() const { return(this->_httpVersion); }

const std::string& HttpRequest::getBody() const { return(this->_body); }

const std::map<std::string, std::string>& HttpRequest::getHeaders() const { return(this->_headers); }

size_t HttpRequest::getContentLength() const { return (this->_contentLength); }

bool HttpRequest::isChunked() const { return (this->_isChunked); }

std::string HttpRequest::getHeader(const std::string& key) const
{
	std::map<std::string, std::string>::const_iterator it = this->_headers.find(key);
    if (it != this->_headers.end()) {
        return it->second;
    }
    return "";
}

bool HttpRequest::hasHeader(const std::string& key) const
{
	std::map<std::string, std::string>::const_iterator it = this->_headers.find(key);
    if (it != this->_headers.end()) {
        return true;
    }
    return false;
}


void HttpRequest::clear()
{
	this->_method.clear();
	this->_uri.clear();
	this->_path.clear();
	this->_queryString.clear();
	this->_httpVersion.clear();
	this->_headers.clear();
	this->_body.clear();
	this->_state = PARSING_REQUEST_LINE;
	this->_errorCode = 0;
	this->_contentLength = 0;
	this->_isChunked = false;
}
