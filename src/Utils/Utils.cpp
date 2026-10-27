#include "Utils/Utils.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <limits>

static const std::string TCHAR_SYMBOLS = "!#$%&'*+-.^_`|~";

static int safeToLower( int c ) {
    return std::tolower( static_cast<unsigned char>(c) );
}

std::string toLower( std::string s ) {
	std::transform( s.begin(), s.end(), s.begin(), safeToLower );
	return s;
}

std::string trim( const std::string & s ) {
	size_t first = s.find_first_not_of(" \t");
	if ( first == std::string::npos )
		return "";

	size_t second = s.find_last_not_of(" \t");

	return s.substr(first, second - first + 1);
}

std::vector<std::string> split( const std::string & s, const std::string & delimiter ) {
	std::vector<std::string> res;

	if ( delimiter.empty() ) {
        res.push_back( s );
        return res;
    }

	size_t start = 0;
	size_t end = s.find( delimiter );

	while ( end != std::string::npos ) {
		res.push_back( s.substr(start, end - start) );
		start = end + delimiter.size();
		end = s.find( delimiter, start );
    }

	res.push_back( s.substr(start) );
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

bool isTchar(char c) {
    return isDigit(c) || isUpper(c) || isLower(c)
		|| TCHAR_SYMBOLS.find(c) != std::string::npos;
}

bool isToken( const std::string & s ) {
	if (s.empty())
		return false;
		
	for ( size_t i = 0; i < s.size(); i++ ) {
		if ( !isTchar(s[i]) )
			return false;
	}
	return true;
}

bool stringToSize( const std::string & s, int base, size_t & n ) {
	if ( s.empty() || ( base != 10 && base != 16 ) )
		return false;

	size_t result = 0;
	for ( size_t i = 0; i < s.size(); i++ ) {
		char c = s[i];
		int digit;

		if ( c >= '0' && c <= '9' )
			digit = c - '0';
		else if ( c >= 'a' && c <= 'f' )
			digit = c - 'a' + 10;
		else if ( c >= 'A' && c <= 'F' )
			digit = c - 'A' + 10;
		else
			return false;

		if ( digit >= base )
			return false;
		if ( result > ( std::numeric_limits<size_t>::max() - digit ) / base )
			return false;

		result = result * base + digit;
	}

	n = result;
	return true;
}

bool hasControlChars( const std::string & s ) {
	for ( size_t i = 0; i < s.size(); i++ ) {
		unsigned char c = s[i];
		if ( (c < ' ' && c != '\t') || c >= 127 )
			return true;
	}

	return false;
}

int charToInt( char c ) {
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return c - '0';
}
