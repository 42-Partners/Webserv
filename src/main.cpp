#include <iostream>
#include <stdexcept>

#include "parser.hpp"
#include "HttpRequest/HttpRequest.hpp"

int main( int ac, char** av ) {
	try {
		arg_parser( ac, av );
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	HttpRequest req;
	std::string ExampleRawBuffer = "GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: Mozilla/5.0\r\nAccept: text/html\r\n\r\n";
	parseIncomingBuffer(req, ExampleRawBuffer);

	std::cout << req.getMethod() << std::endl;
	std::cout << req.getUri() << std::endl;
	std::cout << req.getPath() << std::endl;
	std::cout << req.getQueryString() << std::endl;
	std::cout << req.getHttpVersion() << std::endl;


	std::cout << req.getHeader("host") << std::endl;
	std::cout << req.hasHeader("host") << std::endl;
	// std::cout << req.getHeaders() << std::endl;
	std::cout << req.getBody() << std::endl;
	std::cout << req.getState() << std::endl;
	std::cout << req.getErrorCode() << std::endl;
	std::cout << req.getContentLength() << std::endl;
	std::cout << req.isChunked() << std::endl;

	return 0;
}