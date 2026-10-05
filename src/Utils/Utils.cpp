#include "Utils/Utils.hpp"

#include <algorithm>
#include <cctype>

std::string toLower( std::string s ) {
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}