#include "Logger/Logger.hpp"
#include "Utils/Utils.hpp"

#include <iostream>

void Logger::info( const std::string & module, const std::string & message ) {
	std::cout << "[INFO]" << "[" << toUpper(module) << "] " << message << std::endl;
}

void Logger::error( const std::string & module, const std::string & message ) {
	std::cerr << "[ERROR]" << "[" << toUpper(module) << "] " << message << std::endl;
}