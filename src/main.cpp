#include "parser.hpp"
#include <iostream>
#include <stdexcept>

int main( int ac, char** av ) {
	try {
		arg_parser( ac, av );
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}