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