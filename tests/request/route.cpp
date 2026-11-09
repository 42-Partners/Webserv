#include "HttpResponse/HttpResponse.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "RequestProcessor/RequestProcessor.hpp"
#include "config/ServerConfig.hpp"
#include "config/LocationConfig.hpp"
#include "StatusCode/StatusCode.hpp"
#include "../tests.hpp"

#include <string>
#include <fstream>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

// --------------------------------------------------
// Helpers (ADAPTE ao seu codigo)
// --------------------------------------------------

static const std::string TEST_ROOT = "/tmp/webserv_route_test";

static bool isSuccess( int status ) {
	return status >= 200 && status < 300;
}

static bool fileExists( const std::string & path ) {
	struct stat st;
	return stat( path.c_str(), &st ) == 0;
}

static bool writeFile( const std::string & path, const std::string & content ) {
	std::ofstream f( path.c_str() );
	if ( !f.is_open() )
		return false;
	f << content;
	return true;
}

// ADAPTE: como voce alimenta o parser com uma request crua
static HttpRequest makeRequest( std::string raw ) {
	HttpRequest request;
	request.feed( raw );
	return request;
}

// ADAPTE: como voce constroi uma LocationConfig
static LocationConfig makeLocation( const std::string & path,
									const std::string & root,
									const char * a,
									const char * b = NULL,
									const char * c = NULL ) {
	LocationConfig loc;
	loc.setPath( path );
	loc.setRoot( root );
	loc.addAllowedMethod( a );
	if ( b ) loc.addAllowedMethod( b );
	if ( c ) loc.addAllowedMethod( c );
	return loc;
}

// "/"    -> TEST_ROOT, GET POST DELETE
// "/ro"  -> TEST_ROOT, so GET
static ServerConfig makeServer() {
	ServerConfig config;
	config.addLocation( makeLocation( "/",   TEST_ROOT, "GET", "POST", "DELETE" ) );
	config.addLocation( makeLocation( "/ro", TEST_ROOT, "GET" ) );
	return config;
}

// ServerConfig sem location "/", para forcar 404 de location
static ServerConfig makeServerWithoutRoot() {
	ServerConfig config;
	config.addLocation( makeLocation( "/ro", TEST_ROOT, "GET" ) );
	return config;
}

static std::string get( const std::string & path ) {
	return "GET " + path + " HTTP/1.1\r\nHost: x\r\n\r\n";
}

static std::string del( const std::string & path ) {
	return "DELETE " + path + " HTTP/1.1\r\nHost: x\r\n\r\n";
}

static const std::string FILE_CONTENT = "hello route test\n";

static void setupEnv() {
	mkdir( TEST_ROOT.c_str(), 0755 );
	writeFile( TEST_ROOT + "/a.txt", FILE_CONTENT );
	writeFile( TEST_ROOT + "/to_delete.txt", "bye" );
}

static void teardownEnv() {
	std::remove( ( TEST_ROOT + "/a.txt" ).c_str() );
	std::remove( ( TEST_ROOT + "/to_delete.txt" ).c_str() );
	rmdir( TEST_ROOT.c_str() );
}

// --------------------------------------------------
// Erros vindos do parser
// --------------------------------------------------

static void testRouteParserError() {
	ServerConfig config = makeServer();
	HttpRequest bad = makeRequest( "GARBAGE\r\n\r\n" );
	HttpResponse r = process( bad, config );

	check( r.getStatusCode() == bad.getErrorCode(), "route: parser error uses parser code" );
	check( r.shouldClose(),                         "route: parser error closes connection" );
	check( !r.getBody().empty(),                    "route: parser error has body" );
}

static void testRouteUnknownMethod() {
	ServerConfig config = makeServer();
	HttpRequest req = makeRequest( "PATCH / HTTP/1.1\r\nHost: x\r\n\r\n" );
	HttpResponse r = process( req, config );

	check( r.getStatusCode() == STATUS_NOT_IMPLEMENTED, "route: unknown method -> 501" );
	check( r.shouldClose(),                             "route: 501 from parser closes connection" );
}

static void testRoutePathTraversal() {
	ServerConfig config = makeServer();
	HttpRequest req = makeRequest( get( "/../etc/passwd" ) );
	HttpResponse r = process( req, config );

	check( r.isError(),      "route: path traversal is rejected" );
	check( r.shouldClose(),  "route: path traversal rejected by parser closes connection" );
}

// --------------------------------------------------
// Location e metodo
// --------------------------------------------------

static void testRouteNoLocation() {
	ServerConfig config = makeServerWithoutRoot();
	HttpRequest req = makeRequest( get( "/other" ) );
	HttpResponse r = process( req, config );

	check( r.getStatusCode() == STATUS_NOT_FOUND, "route: no location -> 404" );
	check( !r.shouldClose(),                      "route: 404 keeps connection" );
	check( !r.getBody().empty(),                  "route: 404 has body" );
}

static void testRouteMethodNotAllowed() {
	setupEnv();
	ServerConfig config = makeServer();
	HttpRequest req = makeRequest( del( "/ro/to_delete.txt" ) );
	HttpResponse r = process( req, config );

	check( r.getStatusCode() == STATUS_METHOD_NOT_ALLOWED, "route: method not allowed -> 405" );
	check( !r.shouldClose(),                               "route: 405 keeps connection" );
	check( !r.getBody().empty(),                           "route: 405 has body" );
	check( fileExists( TEST_ROOT + "/to_delete.txt" ),     "route: 405 does not run the handler (file intact)" );
	teardownEnv();
}

// --------------------------------------------------
// GET
// --------------------------------------------------

static void testRouteGetExistingFile() {
	setupEnv();
	ServerConfig config = makeServer();
	HttpRequest req = makeRequest( get( "/a.txt" ) );
	HttpResponse r = process( req, config );

	check( r.getStatusCode() == 200,      "route: GET existing file -> 200" );
	check( r.getBody() == FILE_CONTENT,   "route: GET body equals file content" );
	check( !r.shouldClose(),              "route: GET 200 keeps connection" );
	teardownEnv();
}

static void testRouteGetMissingFile() {
	setupEnv();
	ServerConfig config = makeServer();
	HttpRequest req = makeRequest( get( "/missing.txt" ) );
	HttpResponse r = process( req, config );

	check( r.getStatusCode() == STATUS_NOT_FOUND, "route: GET missing file -> 404" );
	check( !r.getBody().empty(),                  "route: GET 404 has body" );
	check( !r.shouldClose(),                      "route: GET 404 keeps connection" );
	teardownEnv();
}

static void testRouteGetViaPrefixLocation() {
	setupEnv();
	ServerConfig config = makeServer();
	HttpRequest req = makeRequest( get( "/ro/a.txt" ) );
	HttpResponse r = process( req, config );

	check( r.getStatusCode() == 200,    "route: GET through /ro location -> 200" );
	check( r.getBody() == FILE_CONTENT, "route: GET through /ro location serves the file" );
	teardownEnv();
}

// --------------------------------------------------
// DELETE
// --------------------------------------------------

static void testRouteDeleteExistingFile() {
	setupEnv();
	ServerConfig config = makeServer();
	HttpRequest req = makeRequest( del( "/to_delete.txt" ) );
	HttpResponse r = process( req, config );

	check( isSuccess( r.getStatusCode() ),               "route: DELETE existing file -> 2xx" );
	check( !fileExists( TEST_ROOT + "/to_delete.txt" ),  "route: DELETE removes the file from disk" );
	check( fileExists( TEST_ROOT + "/a.txt" ),           "route: DELETE leaves other files alone" );
	teardownEnv();
}

static void testRouteDeleteMissingFile() {
	setupEnv();
	ServerConfig config = makeServer();
	HttpRequest req = makeRequest( del( "/missing.txt" ) );
	HttpResponse r = process( req, config );

	check( r.getStatusCode() == STATUS_NOT_FOUND, "route: DELETE missing file -> 404" );
	check( !r.getBody().empty(),                  "route: DELETE 404 has body" );
	teardownEnv();
}

// --------------------------------------------------
// POST
// --------------------------------------------------
// ADAPTE: depende do contrato do handlePost. Aqui so garanto que o
// despacho chega nele (nao e 405, nao e 404 de location, nao e 500).

static void testRoutePostIsDispatched() {
	setupEnv();
	ServerConfig config = makeServer();
	HttpRequest req = makeRequest(
		"POST /a.txt HTTP/1.1\r\nHost: x\r\nContent-Length: 0\r\n\r\n" );
	HttpResponse r = process( req, config );

	check( r.getStatusCode() != STATUS_METHOD_NOT_ALLOWED,    "route: POST allowed is not 405" );
	check( r.getStatusCode() != STATUS_INTERNAL_SERVER_ERROR, "route: POST does not hit the 500 default" );
	teardownEnv();
}

// --------------------------------------------------
// Runner
// --------------------------------------------------

void runRouteTests() {
	RUN( testRouteParserError );
	RUN( testRouteUnknownMethod );
	RUN( testRoutePathTraversal );
	RUN( testRouteNoLocation );
	RUN( testRouteMethodNotAllowed );
	RUN( testRouteGetExistingFile );
	RUN( testRouteGetMissingFile );
	RUN( testRouteGetViaPrefixLocation );
	RUN( testRouteDeleteExistingFile );
	RUN( testRouteDeleteMissingFile );
	RUN( testRoutePostIsDispatched );
}