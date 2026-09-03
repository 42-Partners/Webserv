#include <iostream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

static void file_handling( const char *file );

void arg_parser( int ac, char** av ) {
	switch ( ac ) {
		case 1:
			file_handling( "config/default.conf" );
			std::cout << "config carregado com sucesso"<< std::endl;
			break;
		case 2:
			file_handling( av[1] );
			std::cout << "config carregado com sucesso"<< std::endl;
			break;
		default:
			throw std::runtime_error("Usage: ./build/webserv\nor: ./build/webserv <config>"); 
	}
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