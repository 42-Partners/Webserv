#include "HttpRequest/HttpRequest.hpp"

/*
400 Bad Request ->	linha de requisição malformada, header sem :, Content-Length inválido, chunk com tamanho inválido, Host ausente em HTTP/1.1
413 Content Too Large -> body maior que o client_max_body_size
414 URI Too Long -> linha de requisição acima do limite
431 Request Header Fields Too Large -> bloco de headers acima do limite
501 Not Implemented -> método que você não conhece (PATCH, FOO...)
505 HTTP Version Not Supported -> versão diferente de 1.0 e 1.1
411 Length Required -> POST sem Content-Length e sem chunked.
*/

/*
POST /upload/foto.png?user=ana HTTP/1.1\r\nHost: localhost:8080\r\nContent-Type: image/png\r\nContent-Length: 12\r\n\r\nconteudo....
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

// procura o primeiro CRLF. Se não achou, não consome nada e pede mais dados. Se achou, consome a linha e a valida.
bool HttpRequest::parseRequestLine( std::string & buffer ) {
	(void)buffer;
	return true;
}

// procura o fim do bloco (\r\n\r\n). Se achou, consome o bloco e interpreta linha a linha.
bool HttpRequest::parseHeaders( std::string & buffer ) {
	(void)buffer;
	return true;
}

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

void HttpRequest::setError( int code ) {
	(void)code;
}

bool HttpRequest::finishHeaders() {
	return true;
}

bool HttpRequest::parseHeaderLine( const std::string & line ) {
	(void)line;
	return true;
}

bool HttpRequest::parseUri( const std::string & rawUri ) {
	//!decodifica: Depois do % vêm dois dígitos hexadecimais, e eles formam o valor do byte. %20 é o byte 0x20 (espaço), e %41 é o byte 0x41 (A). Verificar casos de erro.
	(void)rawUri;
	return true;
} 
