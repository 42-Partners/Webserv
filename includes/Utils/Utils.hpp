#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

std::string toLower( std::string );

std::vector<std::string> split( std::string line, const std::string & delimiter );

bool isDigit( char c );
bool isHexDigit( char c );
bool isUpper( char c );

int hexaToInt( char c );

#endif