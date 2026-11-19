#include "Handlers/RedirectHandler.hpp"

HttpResponse handleRedirect( const LocationConfig & loc ) {
	const std::pair<int, std::string> & redirect = loc.getRedirect();
	
	return makeRedirect(redirect.first, redirect.second);
}

HttpResponse makeRedirect( int status, const std::string & target ) {
	HttpResponse response;

	response.setStatus(status);
	response.setHeader("Location", target);
	response.setBody("<html><body>Redirecting to " + target + "</body></html>");

	return response;
}
