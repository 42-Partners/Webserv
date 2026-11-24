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
	
	if ( request.getMethod() == "HEAD" && response.getStatusCode() < STATUS_BAD_REQUEST ) {
		if ( !response.hasHeader( "Content-Length" ) ) {
			std::ostringstream lenSs;
			lenSs << response.getBody().size();
			response.setHeader( "Content-Length", lenSs.str() );
		}
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

	if ( method == "HEAD" )
		return METHOD_HEAD;

	if ( method == "DELETE" )
		return METHOD_DELETE;

	if ( method == "POST" )
		return METHOD_POST;

	return METHOD_UNKNOWN;
}

static bool isCgiRequest(const std::string& uriPath, const LocationConfig& location) {
	std::string cleanPath = uriPath;
	size_t queryPos = cleanPath.find('?');
	if (queryPos != std::string::npos) {
		cleanPath = cleanPath.substr(0, queryPos);
	}

	const std::string& cgiExt = location.getCgiExtension();
	if (cgiExt.empty()) {
		return false;
	}

	size_t pos = cleanPath.find(cgiExt);
	while (pos != std::string::npos) {
		size_t endPos = pos + cgiExt.size();
		if (endPos == cleanPath.size() || cleanPath[endPos] == '/') {
			return true;
		}
		pos = cleanPath.find(cgiExt, pos + 1);
	}

	return false;
}

static HttpResponse route( const HttpRequest & request, const ServerConfig & config ) {
	if ( request.getState() == HttpRequest::REQUEST_ERROR ) {
		HttpResponse response = handleError( config, request.getErrorCode() );
		response.setKeepAlive( false );
		return response;
	}

	const LocationConfig * loc = resolveLocation( request.getPath(), config );
	if  ( !loc )
		return handleError( config, STATUS_NOT_FOUND );

	if (loc->hasRedirect()) {
		Logger::info("Request Processor",
					"Redirect " + request.getPath() + " -> " + loc->getRedirect().second);
					
		return handleRedirect(*loc);
	}

	bool methodAllowed = loc->isMethodAllowed( request.getMethod() )
						 || ( request.getMethod() == "HEAD" && loc->isMethodAllowed( "GET" ) );

	if ( !methodAllowed ) {
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
