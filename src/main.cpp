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

	return 0;
}