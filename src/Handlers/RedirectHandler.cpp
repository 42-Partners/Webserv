#include "Handlers/RedirectHandler.hpp"

HttpResponse handleRedirect( const LocationConfig & loc ) {
	HttpResponse response;

	const std::pair<int, std::string> & redirect = loc.getRedirect();

	response.setStatus( redirect.first );
	response.setHeader( "Location", redirect.second );

	return response;
}
