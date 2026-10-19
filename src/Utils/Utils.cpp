#include "Utils/Utils.hpp"

#include <algorithm>
#include <cctype>

static int safeToLower( int c ) {
    return std::tolower( static_cast<unsigned char>(c) );
}

std::string toLower( std::string s ) {
	std::transform( s.begin(), s.end(), s.begin(), safeToLower );
	return s;
}

std::string trim( std::string s ) {
	size_t first = s.find_first_not_of(" \t");
	if ( first == std::string::npos )
		return "";

	size_t second = s.find_last_not_of(" \t");

	return s.substr(first, second - first + 1);
}

std::vector<std::string> split( std::string s, const std::string & delimiter ) {
	
	std::vector<std::string> res;

	if ( !delimiter.empty() ) {

		size_t pos = s.find( delimiter );

		while ( pos != std::string::npos ) {
			res.push_back( s.substr(0, pos) );
			s.erase( 0, pos + delimiter.size() );
			pos = s.find( delimiter );
		}
	}

	res.push_back( s );

	return res;
}

bool isDigit( char c ) {
	return c >= '0' && c <= '9'; 
}

bool isHexDigit( char c ) {
	return isDigit( c )
		|| (c >= 'A' && c <= 'F')
		|| (c >= 'a' && c <= 'f');
}

bool isUpper( char c ) {
	return c >= 'A' && c <= 'Z'; 
}

bool isLower( char c ) {
	return c >= 'a' && c <= 'z'; 
}

bool isAllDigits( const std::string & s) {
	if ( s.empty() )
		return false;

	for (size_t i = 0; i < s.size(); i++) {
		if ( !isDigit(s[i]) )
			return false;
	}

	return true;
}

int hexCharToInt( char c ) {
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return c - '0';
}
