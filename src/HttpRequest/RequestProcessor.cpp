#include "config/LocationConfig.hpp"
#include "Handlers/CgiHandler.hpp"
#include "Handlers/ErrorHandler.hpp"
#include "Handlers/RedirectHandler.hpp"
#include "Handlers/StaticFileHandler.hpp"
#include "Handlers/UploadHandler.hpp"
#include "RequestProcessor/RequestProcessor.hpp"
#include "Logger/Logger.hpp"
#include "StatusCode/StatusCode.hpp"

#include <ctime>
#include <string>
#include <vector>

static HttpResponse route( const HttpRequest & request, const ServerConfig & config );
static void finalizeResponse( HttpResponse & response );

HttpResponse process( const HttpRequest & request, const ServerConfig & config ) { 
	HttpResponse response = route( request, config );

	if ( request.getMethod() == "HEAD" ) {
		std::ostringstream ss;
		ss << response.getBody().size();
		response.setHeader( "Content-Length", ss.str() );
		response.setBody( "" );
	}
		
	if ( !request.getKeepAlive() )
		response.setKeepAlive( false );

	finalizeResponse( response );

	std::ostringstream ss;
	ss << response.getStatusCode();

	if (request.getState() == HttpRequest::REQUEST_COMPLETE)
    	Logger::info( "Request Processor",
					request.getMethod() + " " + request.getPath() + " -> " + ss.str() );

	return response;
}

static Methods methodFromString( const std::string & method ) {
	if ( method == "GET" )
		return METHOD_GET;

	if ( method == "HEAD" )
		return METHOD_HEAD;

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

	const LocationConfig * loc = config.resolveLocation( request.getPath() );
	if  ( !loc )
		return handleError( config, STATUS_NOT_FOUND );

	if (loc->hasRedirect()) {
		Logger::info("Request Processor",
					"Redirect " + request.getPath() + " -> " + loc->getRedirect().second);
					
		return handleRedirect(*loc);
	}

	if ( !loc->isMethodAllowed( request.getMethod() ) ) {
		HttpResponse response = handleError( config, STATUS_METHOD_NOT_ALLOWED );
		response.setHeader( "Allow", loc->getAllowedMethodsString() );
		return response;
	}

	if ( isCgiRequest(request.getPath(), *loc) )
		return handleCgi( request, config, *loc );

	switch ( methodFromString( request.getMethod() ) ) {
		case METHOD_GET:
		case METHOD_HEAD:
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

	if ( !response.getKeepAlive() ) {
		Logger::info("Request Processor", "Connection will be closed");
		response.setHeader( "Connection", "close" );
	}
}
