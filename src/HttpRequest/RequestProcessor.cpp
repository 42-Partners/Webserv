#include "config/LocationConfig.hpp"
#include "StatusCode/StatusCode.hpp"
#include "HttpResponse/ErrorPage.hpp"
#include "RequestProcessor/RequestProcessor.hpp"

#include <fstream>
#include <sstream>

static bool loadErrorPage( const std::string & path, std::string & page );
static HttpResponse resolveErrorPage( int errorCode, const ServerConfig & config );
static const LocationConfig * resolveLocation( const std::string & requestPath, const ServerConfig & config );
static std::string resolveFilePath( HttpResponse & response, const std::string & requestPath, const LocationConfig & location );

HttpResponse process( const HttpRequest & request, const ServerConfig & config ) { 
	HttpResponse response;

	if ( request.getState() == HttpRequest::REQUEST_ERROR )
		response = resolveErrorPage( request.getErrorCode(), config );

	else if ( request.getState() == HttpRequest::REQUEST_COMPLETE ) {
		const LocationConfig * location = resolveLocation( request.getPath(), config );
		if  ( !location )
			response = resolveErrorPage( 404, config );
		else {
			std::string file = resolveFilePath( response, request.getPath(), *location );
			// HttpResponse buildResponse()
			if (response.isError() )
				response = resolveErrorPage( response.getStatusCode(), config );
		}
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

static const LocationConfig * resolveLocation( const std::string & requestPath, const ServerConfig & config ) {
	const LocationConfig * location = NULL;

	const std::vector<LocationConfig> & locConfig = config.getLocations();

	size_t bestSize = 0;
	for (size_t i = 0; i < locConfig.size(); i++ ) {
		const std::string & locPath = locConfig[i].getPath();

		if ( locPath.empty() ) continue;

		if ( requestPath.size() < locPath.size() ) continue;

		size_t size = locPath.size();

		if ( requestPath.compare( 0, size, locPath ) ) continue;

		if ( locPath[size-1] != '/' &&
			 requestPath.size() != size &&
			 requestPath[size] != '/' ) continue;

		if ( !location || size > bestSize ) {
			bestSize = size;
			location = &locConfig[i];
		}
	}

	return location;
}

static std::string resolveFilePath( HttpResponse & response, const std::string & requestPath, const LocationConfig & location ) {
	const std::string leftover = requestPath.substr( location.getPath().size() );
	const std::string & root = location.getRoot();

	if ( root.empty() ) {
		response.setStatus( STATUS_INTERNAL_SERVER_ERROR );
		return "";
	}

	if ( leftover.empty() )
		return root;

	bool rootEndsWithSlash = root[root.size() - 1] == '/';
	bool leftoverStartsWithSlash = leftover[0] == '/';

	std::string filePath;
	if ( !rootEndsWithSlash && !leftoverStartsWithSlash )
		filePath = root + "/" + leftover;
	else if ( rootEndsWithSlash && leftoverStartsWithSlash )
		filePath = root + leftover.substr( 1 );
	else 
		filePath = root + leftover;

	return filePath;
}
