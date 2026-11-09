#include "config/LocationConfig.hpp"
#include "Handlers/ErrorHandler.hpp"
#include "Handlers/RedirectHandler.hpp"
#include "Handlers/StaticFileHandler.hpp"
#include "Handlers/UploadHandler.hpp"
#include "RequestProcessor/RequestProcessor.hpp"
#include "StatusCode/StatusCode.hpp"

#include <cassert>
#include <ctime>
#include <string>
#include <vector>

static HttpResponse route( const HttpRequest & request, const ServerConfig & config );
static void finalizeResponse( HttpResponse & response );

HttpResponse process( const HttpRequest & request, const ServerConfig & config ) { 
	HttpResponse response = route( request, config );

	if ( !request.getKeepAlive() )
		response.setKeepAlive( false );

	finalizeResponse( response );

	return response;
}
// assume path nao vazio e começando com /
static const LocationConfig * resolveLocation( const std::string & requestPath, const ServerConfig & config ) {
	const LocationConfig * loc = NULL;

	const std::vector<LocationConfig> & locConfig = config.getLocations();

	size_t bestSize = 0;
	for (size_t i = 0; i < locConfig.size(); i++ ) {
		const std::string & locPath = locConfig[i].getPath();

		if ( requestPath.size() < locPath.size() ) continue;

		size_t size = locPath.size();

		if ( requestPath.compare( 0, size, locPath ) ) continue;

		bool boundaryOk = locPath[size-1] == '/'
						|| requestPath.size() == size
						|| requestPath[size] == '/';
		if ( !boundaryOk ) continue;

		if ( size > bestSize ) {
			bestSize = size;
			loc = &locConfig[i];
		}
	}

	return loc;
}

static Methods methodFromString( const std::string & method ) {
	if ( method == "GET" )
		return METHOD_GET;

	if ( method == "DELETE" )
		return METHOD_DELETE;

	if ( method == "POST" )
		return METHOD_POST;

	return METHOD_UNKNOWN;
}

static HttpResponse route( const HttpRequest & request, const ServerConfig & config ) {
	if ( request.getState() == HttpRequest::REQUEST_ERROR ) {
		HttpResponse response = handleError( config, request.getErrorCode() );
		response.setKeepAlive( false );
		return response;
	}

	assert( request.getState() == HttpRequest::REQUEST_COMPLETE ); // tirar antes da entrega

	const LocationConfig * loc = resolveLocation( request.getPath(), config );
	if  ( !loc )
		return handleError( config, STATUS_NOT_FOUND );

	if ( loc->hasRedirect() )
		return handleRedirect( *loc );

	if ( !loc->isMethodAllowed( request.getMethod() ) ) {
		HttpResponse response = handleError( config, STATUS_METHOD_NOT_ALLOWED );
		response.setHeader( "Allow", loc->getAllowedMethodsString() );
		return response;
	}

	switch ( methodFromString( request.getMethod() ) ) {
		case METHOD_GET: 
			return handleGet( request, config, *loc );
		case METHOD_DELETE:
			return handleDelete( request, config, *loc );
		case METHOD_POST:
			return handlePost( request, config, *loc );
		default:
			return handleError( config, STATUS_INTERNAL_SERVER_ERROR );
	}
}

static void finalizeResponse( HttpResponse & response ) {
	if ( !response.getBody().empty() ) {
		if ( !response.hasHeader( "Content-Type" ) )
			response.setHeader( "Content-Type", "application/octet-stream" );
	}

	response.setHeader( "Server", "42webserver" );

	time_t timestamp = std::time( NULL );
	struct tm * utc = std::gmtime( &timestamp );
	char date[64];

	if ( utc && std::strftime( date, sizeof( date ), "%a, %d %b %Y %H:%M:%S GMT", utc ) > 0 )
		response.setHeader( "Date", date );

	if ( !response.getKeepAlive() )
		response.setHeader( "Connection", "close" );
}
