#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

std::string toLower( std::string );
std::string trim( std::string );

std::vector<std::string> split( std::string, const std::string & );

bool isDigit( char );
bool isHexDigit( char );
bool isUpper( char );
bool isLower( char );
bool isAllDigits( const std::string & );

int hexCharToInt( char );

#endif