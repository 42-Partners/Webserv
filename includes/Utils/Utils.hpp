#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

std::string toLower( std::string );
std::string trim( const std::string & s );

std::vector<std::string> split( const std::string &, const std::string & );

bool isDigit( char );
bool isHexDigit( char );
bool isUpper( char );
bool isLower( char );
bool isTchar(char c);
bool isToken( const std::string & );
bool stringToSize( const std::string &, int, size_t & );
bool hasControlChars( const std::string & );

int charToInt( char );

#endif