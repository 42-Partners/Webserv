#include <iostream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include "config/Tokenizer.hpp"
#include "config/Parser.hpp"

static void file_handling( const char *file );

void arg_parser( int ac, char** av ) {
	std::string file = "config/default.conf";
	switch ( ac ) {
		case 1:
			break;
		case 2:
			file = av[1];
			break;
		default:
			throw std::runtime_error("Usage: ./build/webserv\nor: ./build/webserv <config>"); 
	}
	file_handling(file.c_str());
	std::cout << "config carregado com sucesso"<< std::endl;
	Parser::parse_tokens(Tokenizer::tokenize(file));
}

static void file_handling( const char *file ) {
	if ( access(file, F_OK) || access(file, R_OK) )
		throw std::runtime_error("access(): File does not exist or is inaccessible."); 

	struct stat fileInfo;

	if (stat(file, &fileInfo) == 0) {
		if (!S_ISREG(fileInfo.st_mode))
			throw std::runtime_error("stat(): the file is not a regular file");
	}
	else
		throw std::runtime_error("stat(): Failed.");
}