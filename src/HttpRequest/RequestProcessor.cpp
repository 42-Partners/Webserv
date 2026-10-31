#include "config/LocationConfig.hpp"
#include "HttpResponse/ErrorPage.hpp"
#include "RequestProcessor/RequestProcessor.hpp"

#include <fstream>
#include <sstream>

static bool loadErrorPage( const std::string & path, std::string & page );
static HttpResponse resolveErrorPage( int errorCode, const ServerConfig & config );
static LocationConfig * resolveLocation( const std::string & path, const ServerConfig & config );

HttpResponse process( const HttpRequest & request, const ServerConfig & config ) { 
	HttpResponse response;

	if ( request.getState() == HttpRequest::REQUEST_ERROR )
		response = resolveErrorPage( request.getErrorCode(), config );

	else if ( request.getState() == HttpRequest::REQUEST_COMPLETE ) {
		LocationConfig * location = resolveLocation( request.getPath(), config );
		if  ( !location )
			response = resolveErrorPage( 404, config );
		// criar response
		else if (response.isError() )
			response = resolveErrorPage( response.getStatusCode(), config );
	}

	return response;
}

static bool loadErrorPage( const std::string & path, std::string & page ) {
	std::ifstream file( path.c_str() );

	if ( !file.is_open() )
		return false;
	
	std::ostringstream buffer;
	buffer << file.rdbuf();

	if ( file.bad() || buffer.fail() )
		return false;
	
	page = buffer.str();
	return true;
}

static HttpResponse resolveErrorPage( int errorCode, const ServerConfig & config ) {
	HttpResponse response;
	
	const std::map<int, std::string> & errorPages = config.getErrorPages();
	std::map<int, std::string>::const_iterator it = errorPages.find( errorCode );

	std::string page;
	if ( it != errorPages.end() && loadErrorPage( it->second, page ) )	
		response.setBody( page );
	else
		response.setBody( defaultErrorPage( errorCode ) );

	response.setStatus( errorCode );
	return response;
}

static LocationConfig * resolveLocation( const std::string & path, const ServerConfig & config ) {
	(void)path;
	(void)config;
	return NULL;
}