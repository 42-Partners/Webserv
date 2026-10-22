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

/*
consome do buffer só o que processa;
deixa a sobra para a próxima requisição;
nunca é chamada depois de REQUEST_COMPLETE ou REQUEST_ERROR.
*/
void HttpRequest::feed( std::string & buffer ) {

	while (_state != REQUEST_COMPLETE && _state != REQUEST_ERROR) {

		bool advanced = false;

		if (_state == PARSING_REQUEST_LINE)
			advanced = parseRequestLine(buffer);
		else if (_state == PARSING_HEADERS)
			advanced = parseHeaders(buffer);
		else if (_state == PARSING_BODY)
			advanced = parseBody(buffer);
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

	if (!parseMethod( requestParts[0] ) ||
		!parseUri( requestParts[1] ) ||
		!parseVersion( requestParts[2] ))
		return false;

	_state = PARSING_HEADERS;
	return true;
}

bool HttpRequest::parseMethod( const std::string & rawMethod ) {
	if (rawMethod.empty())
		return setError(STATUS_BAD_REQUEST);

	for ( size_t i = 0; i < sizeof(METHODS) / sizeof(METHODS[0]); i++ ) {
		if ( rawMethod == METHODS[i] ) {
			_method = rawMethod;
			return true;
		}
	}

	for ( size_t i = 0; i < rawMethod.size(); i++ ) {
		if ( !isTchar(rawMethod[i]) )
			return setError( STATUS_BAD_REQUEST );
	}

	return setError( STATUS_NOT_IMPLEMENTED );
}

bool HttpRequest::parseUri( const std::string & rawUri ) {
	if ( rawUri.empty() || rawUri[0] != '/')
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
			if ( i + 2 >= path.size() ||
				!isHexDigit(path[i+1]) ||
				!isHexDigit(path[i+2]) )
				return setError( STATUS_BAD_REQUEST );
			int first = charToInt(path[i+1]);
			int second = charToInt(path[i+2]);
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

	if ( !decodedPath.compare(0, 2, "//") ||
		 !decodedPath.compare(0, 2, "/\\") )
		return setError( STATUS_BAD_REQUEST );

	std::vector<std::string> decodedPathParts = split( decodedPath, "/" );

	for (size_t i = 0; i < decodedPathParts.size(); i++) {
		if ( decodedPathParts[i] == ".." )
			return setError( STATUS_FORBIDDEN );
	}

	for (size_t i = 0; i < query.size(); i++) {
		if (query[i] == '%') {
			if (i + 2 >= query.size() ||
				!isHexDigit(query[i + 1]) ||
				!isHexDigit(query[i + 2]))
				return setError(STATUS_BAD_REQUEST);
	
			i += 2;
		}
		else if (query[i] < 32 || query[i] >= 127) {
			return setError(STATUS_BAD_REQUEST);
		}
	}

	_uri = rawUri;
	_path = decodedPath;
	_queryString = query;

	return true;
}

bool HttpRequest::parseVersion( const std::string & rawVersion ) {

	if ( rawVersion == "HTTP/1.1" ) {
		_httpVersion = rawVersion;
		return true;
	}

	if ( rawVersion.size() != 8 )
		return setError( STATUS_BAD_REQUEST );

	if ( rawVersion.compare(0, 5, "HTTP/") )
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

	std::pair< map_iterator, bool > res = _headers.insert( std::make_pair(key, value) );

	if ( !res.second ) {
		if (key == "host" ||
			key == "content-length" ||
			key == "transfer-encoding" ||
			key == "content-type")
			return setError( STATUS_BAD_REQUEST );
		else {
			if ( !value.empty() )
				res.first->second += "," + value;
		}
	}

	return true;
}
// parsear tranfer-enconding direito -> deixar concatenar e disparar 501
bool HttpRequest::finishHeaders() {
	const_map_iterator pos = _headers.find("host");
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
		size_t length = 0;
		if ( !stringToSize( pos->second, 10, length ) )
				return setError(STATUS_BAD_REQUEST);
			if ( length > _maxBodySize )
				return setError( STATUS_PAYLOAD_TOO_LARGE );
			_hasContentLength = true;
			_contentLength = length;
	}

	pos = _headers.find("connection");
	if ( pos != _headers.end() ) {
		std::vector<std::string> tokens = split(pos->second, ",");

		for ( std::vector<std::string>::iterator it = tokens.begin();
			  it != tokens.end(); it++ ) {
			if ( toLower(*it) == "close" ) {
				_keepAlive = false;
				break;
			}
		}
	}

	if ( _isChunked && _hasContentLength )
		return setError( STATUS_BAD_REQUEST );

	if ( _isChunked || (_hasContentLength && _contentLength > 0) )
		_state = PARSING_BODY;
	else
		_state = REQUEST_COMPLETE;

	return true;
}

bool HttpRequest::parseBody( std::string & buffer ) {
	if (_isChunked)
		return parseBodyChunked( buffer );
	return parseBodyContentLength( buffer );
}

bool HttpRequest::parseBodyContentLength( std::string & buffer ) {
	if ( buffer.size() < _contentLength )
		return false;

	_body = buffer.substr( 0, _contentLength );
	buffer.erase( 0, _contentLength );

	_state = REQUEST_COMPLETE;
	return true;
}
// validar extensao
bool HttpRequest::parseBodyChunked( std::string & buffer ) {
	if ( buffer.empty() )
		return false;
	
	if ( !isHexDigit( buffer[0] ) )
		return setError( STATUS_BAD_REQUEST );

	size_t pos = buffer.find( CRLF );
	if ( pos == std::string::npos ) {
		if ( buffer.size() > MAX_CHUNK_SIZE_LINE )
			return setError( STATUS_BAD_REQUEST );
		return false;
	}

	if ( pos > MAX_CHUNK_SIZE_LINE )
		return setError( STATUS_BAD_REQUEST );

	std::string sizeLine = buffer.substr(0, pos);
	size_t semicolon = sizeLine.find(";");

	std::string sizeStr = sizeLine;
	if ( semicolon != std::string::npos )
		sizeStr = trim(sizeLine.substr(0, semicolon));

	size_t size = 0;
	if ( !stringToSize( sizeStr, 16, size ) )
		return setError( STATUS_BAD_REQUEST );

	if ( _body.size() > _maxBodySize ||
		 _maxBodySize - _body.size() < size )
		return setError( STATUS_PAYLOAD_TOO_LARGE );
	
	if ( size > 0 ) {
		std::string chunk = buffer.substr( pos + 2 );

		if ( chunk.size() < size ||
			 chunk.size() - size < 2 )
			return false;
		
		if ( chunk.compare(size, 2, CRLF) )
			return setError( STATUS_BAD_REQUEST );
		
		_body += buffer.substr( pos + 2, size );
		buffer.erase( 0, (pos + 2) + size + 2 );

		return true;
	}

	return finishBodyChunked( buffer );
}

bool HttpRequest::finishBodyChunked( std::string & buffer ) {
	std::string sizeLine = "0\r\n";

	size_t pos = buffer.find(HEADERS_END);
	if ( pos == std::string::npos ) {
		if ( MAX_HEADERS_SIZE < buffer.size() - sizeLine.size() )
			return setError( STATUS_HEADERS_TOO_LARGE );
		return false;
	}

	if ( pos > 3 && pos - sizeLine.size() > MAX_HEADERS_SIZE )
		return setError( STATUS_HEADERS_TOO_LARGE );
	
	std::vector<std::string> trailers = split( buffer.substr(0, pos), CRLF );
	
	for ( size_t i = 1; i < trailers.size(); i++ ) {
		size_t dots = trailers[i].find(':');
		if ( dots == std::string::npos )
			return setError( STATUS_BAD_REQUEST );
		std::string name = toLower( trailers[i].substr(0, dots) );
		if ( !isValidHeaderName(name) )
			return setError( STATUS_BAD_REQUEST );

		if ( name == "content-length" ||
			 name == "transfer-encoding" )
			return setError( STATUS_BAD_REQUEST );
		
		if ( !isValidHeaderValue(trailers[i].substr(dots + 1)) )
			return setError( STATUS_BAD_REQUEST );
	}

	buffer.erase( 0, pos + HEADERS_END.size() );

	_state = REQUEST_COMPLETE;
	return true;
}

bool HttpRequest::setError( int code ) {
	_errorCode = code;
	_state = REQUEST_ERROR;
	return false;
}

static bool isValidHeaderName(const std::string &s) {
	if ( s.empty() )
		return false;

	for ( std::string::size_type i = 0; i < s.size(); ++i )
	{
		if ( !isTchar(s[i]) )
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
