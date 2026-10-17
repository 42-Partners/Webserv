#include "HttpRequest/HttpRequest.hpp"
#include "StatusCode/StatusCode.hpp"
#include "Utils/Utils.hpp"

#include <sstream>
#include <utility>

static const std::string CRLF = "\r\n";
static const std::string HEADERS_END = "\r\n\r\n";
static const char * const METHODS[] = {"GET", "POST", "DELETE"};

static bool isValidHeaderName( const std::string & s );
static bool isValidHeaderValue( const std::string & s );

// POST /upload/foto.png?user=ana HTTP/1.1\r\nHost: localhost:8080\r\nContent-Type: image/png\r\nContent-Length: 12\r\n\r\nconteudo....

void HttpRequest::feed( std::string & buffer ) { 

	while (_state != REQUEST_COMPLETE && _state != REQUEST_ERROR) {

		bool advanced = false;

		if (_state == PARSING_REQUEST_LINE)
			advanced = parseRequestLine(buffer);
		else if (_state == PARSING_HEADERS)
			advanced = parseHeaders(buffer);
		// else if (_state == PARSING_BODY)
		// 	advanced = parseBody(buffer);
		if (!advanced)
			break;
	}
}

bool HttpRequest::parseRequestLine( std::string & buffer ) {
	size_t pos = buffer.find(CRLF);

	while (pos == 0) {
		buffer.erase(0, CRLF.size());
		pos = buffer.find(CRLF);
	}

	if (pos == std::string::npos) {
		if (buffer.size() > MAX_REQUEST_LINE)
			return setError(STATUS_URI_TOO_LONG);
		return false;
	}
	
	if (pos > MAX_REQUEST_LINE)
		return setError(STATUS_URI_TOO_LONG);

	std::string line = buffer.substr(0, pos);
	buffer.erase(0, pos + CRLF.size());

	std::vector<std::string> requestParts = split( line, " " );

	if (requestParts.size() != 3)
		return setError(STATUS_BAD_REQUEST);

	for (size_t i = 0; i < requestParts.size(); i++) {
		if (requestParts[i].empty())
			return setError(STATUS_BAD_REQUEST);
	}

	if (!parseMethod( requestParts[0] )
	 || !parseUri( requestParts[1] )
	 || !parseVersion( requestParts[2] ))
		return false;

	_state = PARSING_HEADERS;
	return true;
}

bool HttpRequest::parseMethod( const std::string & rawMethod ) {

	for ( size_t i = 0; i < sizeof(METHODS) / sizeof(METHODS[0]); i++ ) {
		if ( rawMethod == METHODS[i] ) {
			_method = rawMethod;
			return true;
		}
	}

	for ( size_t i = 0; i < rawMethod.size(); i++ ) {
		if ( !isUpper(rawMethod[i]) )
			return setError( STATUS_BAD_REQUEST );
	}

	return setError( STATUS_NOT_IMPLEMENTED );
}

bool HttpRequest::parseUri( const std::string & rawUri ) {
	if (rawUri[0] != '/')
		return setError( STATUS_BAD_REQUEST );

	size_t pos = rawUri.find('?');

	std::string path;
	std::string query;

	if ( pos == std::string::npos )
		path = rawUri;
	else {
		path = rawUri.substr(0, pos);
		query = rawUri.substr(pos + 1);
	}

	std::string decodedPath;
	for (size_t i = 0; i < path.size(); i++) {
		if (path[i] == '%') {
			if ( i + 2 >= path.size() || !isHexDigit(path[i+1]) || !isHexDigit(path[i+2]) )
					return setError( STATUS_BAD_REQUEST );
			int first = hexCharToInt(path[i+1]);
			int second = hexCharToInt(path[i+2]);
			int byte = first * 16 + second;
			if (byte < 32 || byte >= 127)
				return setError( STATUS_BAD_REQUEST );
			decodedPath += static_cast<char>(byte);
			i += 2;
		}
		else {
			if ( path[i] < 32 || path[i] >= 127 )
				return setError( STATUS_BAD_REQUEST );
			decodedPath += path[i];
		}
	}

	std::vector<std::string> decodedPathParts = split( decodedPath, "/" );

	for (size_t i = 0; i < decodedPathParts.size(); i++) {
		if ( decodedPathParts[i] == ".." )
			return setError( STATUS_BAD_REQUEST );
	}

	_uri = rawUri;
	_path = decodedPath;
	_queryString = query;

	return true;
}

bool HttpRequest::parseVersion( const std::string & rawVersion ) {

	if (rawVersion == "HTTP/1.0" || rawVersion == "HTTP/1.1") {
		_httpVersion = rawVersion;
		return true;
	}

	if (rawVersion.size() != 8)
		return setError( STATUS_BAD_REQUEST );

	if (rawVersion.compare(0, 5, "HTTP/"))
		return setError( STATUS_BAD_REQUEST );

	if ( !isDigit(rawVersion[5]) || rawVersion[6] != '.' || !isDigit(rawVersion[7]) )
		return setError( STATUS_BAD_REQUEST );

	return setError( STATUS_VERSION_NOT_SUPPORTED );
}

bool HttpRequest::parseHeaders( std::string & buffer ) {
	size_t pos = buffer.find(CRLF);
	if (pos == 0) {
		buffer.erase(0, CRLF.size());
		return finishHeaders();
	}
	
	pos = buffer.find(HEADERS_END);
	if (pos == std::string::npos) {
		if (buffer.size() >= MAX_HEADERS_SIZE)
			return setError(STATUS_HEADERS_TOO_LARGE);
		return false;
	}

	if (pos >= MAX_HEADERS_SIZE)
		return setError(STATUS_HEADERS_TOO_LARGE);

	std::vector<std::string> headerLines = split( buffer.substr(0, pos), CRLF );
	if ( headerLines.size() > MAX_HEADER_COUNT )
		return setError(STATUS_HEADERS_TOO_LARGE);

	for (size_t i = 0; i < headerLines.size(); i++) {
		if (!parseHeaderLine( headerLines[i] ))
			return false;
	}

	buffer.erase(0, pos + HEADERS_END.size());

	return finishHeaders();
}

bool HttpRequest::parseHeaderLine( const std::string & line ) {
	size_t pos = line.find(':');
	if ( pos == std::string::npos )
		return setError( STATUS_BAD_REQUEST );

	std::string key = toLower(line.substr(0, pos));
	if ( !isValidHeaderName( key ) )
		return setError( STATUS_BAD_REQUEST );

	std::string value = trim(line.substr(pos + 1));
	if ( !isValidHeaderValue( value ) )
		return setError( STATUS_BAD_REQUEST );

	std::pair< map_iterator, bool > res;

	res = _headers.insert( std::make_pair(key, value) );

	if ( !res.second ) {
		if (key == "host" ||
			key == "content-length" ||
			key == "transfer-encoding")
			return setError( STATUS_BAD_REQUEST );
		else {
			if ( !value.empty() )
				_headers[key] = _headers[key] + ", " + value;
		}
	}

	return true;
}

bool HttpRequest::finishHeaders() {
	map_iterator pos = _headers.find("host");
	if ( pos == _headers.end() || pos->second.empty() )
		return setError( STATUS_BAD_REQUEST );
	
	pos = _headers.find("transfer-encoding");
	if ( pos != _headers.end() ) {
		if ( toLower(pos->second) == "chunked" )
			_isChunked = true;
		else
			return setError( STATUS_BAD_REQUEST );
	}

	pos = _headers.find("content-length");
	if ( pos != _headers.end() ) {
		if ( isAllDigits(pos->second) ) {
			std::istringstream ss(pos->second);
			size_t length = 0;
			ss >> length;
			if ( ss.fail() )
				return setError( STATUS_BAD_REQUEST );
			if ( length > DEFAULT_MAX_BODY )
				return setError( STATUS_PAYLOAD_TOO_LARGE );
			_hasContentLength = true;
			_contentLength = length;
		}
		else
			return setError( STATUS_BAD_REQUEST );
	}

	if ( _isChunked && _hasContentLength )
		return setError( STATUS_BAD_REQUEST );

	if ( _isChunked || (_hasContentLength && _contentLength > 0) )
		_state = PARSING_BODY;
	else
		_state = REQUEST_COMPLETE;

	return true;
}

// conteudo....
bool HttpRequest::parseBody( std::string & buffer ) {
	if (_isChunked)
		return parseBodyChunked( buffer );
	return parseBodyContentLength( buffer );
}

bool HttpRequest::parseBodyContentLength( std::string & buffer ) {
	(void)buffer;
	return true;
}

bool HttpRequest::parseBodyChunked( std::string & buffer ) {
	(void)buffer;

	return true;
}

bool HttpRequest::setError( int code ) {
	_errorCode = code;
	_state = REQUEST_ERROR;
	return false;
}

static bool isValidHeaderName( const std::string & s ) {
	if (s.empty())
		return false;
	
	for (size_t i = 0; i < s.size(); i++) {
		if (s[i] <= 32 || s[i] >= 127)
			return false;
	}

	return true;
}

static bool isValidHeaderValue( const std::string & s ) {
	for (size_t i = 0; i < s.size(); i++) {
		if ( (s[i] < 32 && s[i] != 9) || s[i] >= 127 )
			return false;
	}

	return true;
}