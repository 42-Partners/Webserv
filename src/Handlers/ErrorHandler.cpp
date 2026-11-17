#include "Handlers/ErrorHandler.hpp"
#include "HttpResponse/ErrorPage.hpp"

#include <fstream>
#include <sstream>

static bool loadErrorPage( const std::string & path, std::string & page );
static std::string errorPageBody( int errorCode, const ServerConfig & config );
static std::string resolveErrorPagePath( const std::string & errorUri, const ServerConfig & config );

HttpResponse handleError( const ServerConfig & config, int error ) {
	HttpResponse response;

	response.setStatus( error );
	response.setBody( errorPageBody( error, config ) );

	if ( error >= 500 )
		response.setKeepAlive( false );

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

	if ( it != errorPages.end() ) {
		std::string fullPath = resolveErrorPagePath( it->second, config );
		std::string page;
		if ( loadErrorPage( fullPath, page ) )
			return page;
	}

	return defaultErrorPage( errorCode );
}

static std::string resolveErrorPagePath( const std::string & errorUri, const ServerConfig & config ) {
	if ( errorUri.empty() )
		return "";

	if ( errorUri.compare( 0, 2, "./" ) == 0 )
		return errorUri;

	std::string root = "./www";
	const std::vector<LocationConfig> & locations = config.getLocations();
	for ( size_t i = 0; i < locations.size(); ++i ) {
		if ( locations[i].getPath() == "/" && !locations[i].getRoot().empty() ) {
			root = locations[i].getRoot();
			break;
		}
	}

	if ( root[root.size() - 1] == '/' && errorUri[0] == '/' )
		return root + errorUri.substr( 1 );
	if ( errorUri[0] != '/' )
		return root + "/" + errorUri;

	return root + errorUri;
}
