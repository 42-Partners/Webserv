#include "StatusCode/StatusCode.hpp"

#include <sstream>

std::string reasonPhrase(int code) {
	switch (code) {
		case STATUS_OK:						return "OK";
		case STATUS_BAD_REQUEST:			return "Bad Request";
		case STATUS_FORBIDDEN:				return "Forbidden";
		case STATUS_NOT_FOUND:				return "Not Found";
		case STATUS_METHOD_NOT_ALLOWED:		return "Method Not Allowed";
		case STATUS_LENGTH_REQUIRED:		return "Length Required";
		case STATUS_PAYLOAD_TOO_LARGE:		return "Content Too Large";
		case STATUS_URI_TOO_LONG:			return "URI Too Long";
		case STATUS_HEADERS_TOO_LARGE:		return "Request Header Fields Too Large";
		case STATUS_NOT_IMPLEMENTED:		return "Not Implemented";
		default:							return "Unknown";
	}
}

std::string defaultErrorPage(int code) {
	std::ostringstream oss;
	oss << code << " " << reasonPhrase(code);
	std::string title = oss.str();

	oss.str("");
	
	oss << "<!DOCTYPE html>\n"
		<< "<html>\n"
		<< "<head><title>" << title << "</title></head>\n"
		<< "<body>\n"
		<< "<h1>" << title << "</h1>\n"
		<< "</body>\n"
		<< "</html>\n";

	return oss.str();
}