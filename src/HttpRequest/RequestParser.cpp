#include "HttpRequest/HttpRequest.hpp"
#include "StatusCode/StatusCode.hpp"
#include "Utils/Utils.hpp"

static const std::string CRLF = "\r\n";
static const char * const METHODS[] = {"GET", "POST", "DELETE"};

/*
400 Bad Request ->	linha de requisição malformada, header sem :, Content-Length inválido, chunk com tamanho inválido, Host ausente em HTTP/1.1
413 Content Too Large -> body maior que o client_max_body_size
414 URI Too Long -> linha de requisição acima do limite
431 Request Header Fields Too Large -> bloco de headers acima do limite
501 Not Implemented -> método que você não conhece (PATCH, FOO...)
505 HTTP Version Not Supported -> versão diferente de 1.0 e 1.1
411 Length Required -> POST sem Content-Length e sem chunked.
*/

// POST /upload/foto.png?user=ana HTTP/1.1\r\nHost: localhost:8080\r\nContent-Type: image/png\r\nContent-Length: 12\r\n\r\nconteudo....

void HttpRequest::feed( std::string & buffer ) { 

	while (_state != REQUEST_COMPLETE && _state != REQUEST_ERROR) {

		bool advanced = false;

		if (_state == PARSING_REQUEST_LINE)
			advanced = parseRequestLine(buffer);
		// else if (_state == PARSING_HEADERS)
		// 	advanced = parseHeaders(buffer);
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
// Host: localhost:8080\r\nContent-Type: image/png -> procura o fim do bloco (\r\n\r\n). Se achou, consome o bloco e interpreta linha a linha.
bool HttpRequest::parseHeaders( std::string & buffer ) {
	(void)buffer;

	return true;
}
// Host: localhost:8080
bool HttpRequest::parseHeaderLine( const std::string & line ) {
	(void)line;

	return true;
}

bool HttpRequest::finishHeaders() {
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

