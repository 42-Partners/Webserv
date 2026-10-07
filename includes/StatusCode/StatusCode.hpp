#ifndef STATUSCODE_HPP
# define STATUSCODE_HPP

#include <string>

enum StatusCode {
	STATUS_OK						= 200,
	STATUS_BAD_REQUEST				= 400,
	STATUS_FORBIDDEN				= 403,
	STATUS_NOT_FOUND				= 404,
	STATUS_METHOD_NOT_ALLOWED		= 405,
	STATUS_PAYLOAD_TOO_LARGE		= 413,
	STATUS_URI_TOO_LONG				= 414,
	STATUS_HEADERS_TOO_LARGE		= 431,
	STATUS_NOT_IMPLEMENTED			= 501,
	STATUS_LENGTH_REQUIRED			= 411,
};

std::string reasonPhrase( int );
std::string defaultErrorPage( int );

#endif