#include "Handlers/ErrorHandler.hpp"
#include "HttpResponse/ErrorPage.hpp"

#include <fstream>
#include <sstream>

static bool loadErrorPage( const std::string & path, std::string & page );
static std::string errorPageBody( int errorCode, const ServerConfig & config );

HttpResponse handleError( const ServerConfig & config, int error ) {
	HttpResponse response;

	response.setStatus( error );
	response.setBody( errorPageBody( error, config ) );

	if ( error >= 500 )
		response.setConnectionState( true );

	response.setHeader( "content-type", "text/html; charset=utf-8" );

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

static std::string errorPageBody( int errorCode, const ServerConfig & config ) {
	const std::map<int, std::string> & errorPages = config.getErrorPages();
	std::map<int, std::string>::const_iterator it = errorPages.find( errorCode );

	std::string page;
	if ( it != errorPages.end() && loadErrorPage( it->second, page ) )
		return page;

	return defaultErrorPage( errorCode );
}
