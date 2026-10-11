#include "HttpRequest/HttpRequest.hpp"
#include "StatusCode/StatusCode.hpp"
#include "Utils/Utils.hpp"

static const std::string CRLF = "\r\n";
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

	if (!parseMethod( requestParts[0] )
	 || !parseUri( requestParts[1] )
	 || !parseVersion( requestParts[2] ))
		return false;

	_state = PARSING_HEADERS;
	return true;
}
// POST
bool HttpRequest::parseMethod( const std::string & rawMethod ) {
	(void)rawMethod;

	return true;
}
// /upload/foto.png?user=ana
bool HttpRequest::parseUri( const std::string & rawUri ) {
	//!decodifica: Depois do % vêm dois dígitos hexadecimais, e eles formam o valor do byte. %20 é o byte 0x20 (espaço), e %41 é o byte 0x41 (A). Verificar casos de erro.
	(void)rawUri;
	
	return true;
}
// HTTP/1.1
bool HttpRequest::parseVersion( const std::string & rawVersion ) {
	(void)rawVersion;

	return true;
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
