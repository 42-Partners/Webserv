#include "HttpRequest/HttpRequest.hpp"

void parseIncomingBuffer(HttpRequest& req, const std::string& rawBuffer) {
	req.setMethod("POST");
	req.setUri("/upload/avatar.png?user=42");
	req.setPath("/upload/avatar.png");
	req.setQueryString("user=42");
	req.setHttpVersion("HTTP/1.1");

	req.addHeader("host", "localhost:8080");
	req.addHeader("content-type", "application/x-www-form-urlencoded");
	req.addHeader("content-length", "13");

	req.setBody("name=42&age=21");

	req.setState(REQUEST_COMPLETE);
	if(rawBuffer.empty() == true)
	{
		std::cout << rawBuffer << std::endl;
	}
}

HttpRequest::HttpRequest()
{
	
}
HttpRequest::~HttpRequest()
{
	
}


void HttpRequest::setMethod(const std::string& method)
{
	this->_method = method;
}

void HttpRequest::setUri(const std::string& uri)
{
	this->_uri = uri;
}

void HttpRequest::setPath(const std::string& path)
{
	this->_path = path;
}

void HttpRequest::setQueryString(const std::string& query)
{
	this->_queryString = query;
}

void HttpRequest::setHttpVersion(const std::string& version)
{
	this->_httpVersion = version;
}

void HttpRequest::addHeader(const std::string& key, const std::string& value)
{
	this->_headers[key] = value;
}

void HttpRequest::setBody(const std::string& body)
{
	this->_body = body;
}

void HttpRequest::appendBody(const std::string& chunk)
{
	this->_body.append(chunk);
}

void HttpRequest::setState(RequestState state)
{
	this->_state = state;
}

void HttpRequest::setErrorCode(int code)
{
	this->_errorCode = code;
}

void HttpRequest::setContentLength(size_t length)
{
	this->_contentLength = length;
}

void HttpRequest::setIsChunked(bool isChunked)
{
	this->_isChunked = isChunked;
}


const std::string& HttpRequest::getMethod() const
{
	return (this->_method);
}

const std::string& HttpRequest::getUri() const
{
	return(this->_uri);
}

const std::string& HttpRequest::getPath() const
{
	return(this->_path);

}

const std::string& HttpRequest::getQueryString() const
{
	return(this->_queryString);

}

const std::string& HttpRequest::getHttpVersion() const
{
	return(this->_httpVersion);

}

std::string        HttpRequest::getHeader(const std::string& key) const
{
	std::map<std::string, std::string>::const_iterator it = this->_headers.find(key);
    if (it != this->_headers.end()) {
        return it->second;
    }
    return "";
}
bool               HttpRequest::hasHeader(const std::string& key) const
{
	std::map<std::string, std::string>::const_iterator it = this->_headers.find(key);
    if (it != this->_headers.end()) {
        return true;
    }
    return false;

}
const std::map<std::string, std::string>& HttpRequest::getHeaders() const
{
	return(this->_headers);

}
const std::string& HttpRequest::getBody() const
{
	return(this->_body);

}
RequestState       HttpRequest::getState() const
{
	return(this->_state);

}
int                HttpRequest::getErrorCode() const
{
	return(this->_errorCode);

}
size_t             HttpRequest::getContentLength() const
{
	return(this->_contentLength);

}
bool               HttpRequest::isChunked() const
{
	return(this->_isChunked);

}