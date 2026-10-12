#include <iostream>
#include <stdexcept>

#include "HttpRequest/HttpRequest.hpp"

int main( int ac, char** av ) {
	
	HttpRequest req;
	std::string ExampleRawBuffer = "POST /upload/foto.png?user=ana HTTP/1.1\r\nHost: localhost:8080\r\nContent-Type: image/png\r\nContent-Length: 12\r\n\r\nconteudo....";
	req.feed(ExampleRawBuffer);

	std::cout << req.getMethod() << std::endl;
	std::cout << req.getUri() << std::endl;
	std::cout << req.getPath() << std::endl;
	std::cout << req.getQueryString() << std::endl;
	std::cout << req.getHttpVersion() << std::endl;
	// std::cout << req.getHeader("host") << std::endl;
	// std::cout << req.hasHeader("host") << std::endl;
	// std::cout << req.getBody() << std::endl;
	// std::cout << req.getState() << std::endl;
	// std::cout << req.getErrorCode() << std::endl;

	return 0;
}