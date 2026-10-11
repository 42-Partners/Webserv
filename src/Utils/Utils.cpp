#include "Utils/Utils.hpp"

#include <algorithm>
#include <cctype>

std::string toLower( std::string s ) {
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}

std::vector<std::string> split( std::string line, const std::string & delimiter ) {
	
	std::vector<std::string> res;

	if ( !delimiter.empty() ) {

		size_t pos = line.find( delimiter );

		while ( pos != std::string::npos ) {
			res.push_back( line.substr(0, pos) );
			line.erase( 0, pos + delimiter.size() );
			pos = line.find( delimiter );
		}
	}
	res.push_back( line );

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

int hexaToInt( char c ) {
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return c - '0';
}