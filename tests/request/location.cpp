#include "config/LocationConfig.hpp"
#include "config/ServerConfig.hpp"
#include "../src/HttpRequest/RequestProcessor.cpp"
#include "../tests.hpp"

#include <iostream>
#include <string>
#include <vector>


// ---------- helpers ----------
static ServerConfig makeConfig( const std::vector<std::string> & paths ) {
	ServerConfig config;
	for ( size_t i = 0; i < paths.size(); i++ ) {
		LocationConfig loc;
		loc.setPath( paths[i] );
		config.addLocation( loc );
	}
	return config;
}

static std::vector<std::string> makePaths1( const char * a ) {
	std::vector<std::string> v;
	v.push_back( a );
	return v;
}

static std::vector<std::string> makePaths3( const char * a, const char * b, const char * c ) {
	std::vector<std::string> v;
	v.push_back( a );
	v.push_back( b );
	v.push_back( c );
	return v;
}

// devolve o path da location escolhida, ou "<NULL>"
static std::string resolvedPath( const std::string & request, const ServerConfig & config ) {
	const LocationConfig * loc = resolveLocation( request, config );
	return loc ? loc->getPath() : std::string( "<NULL>" );
}

// ADAPTE aqui: cria uma location com path e root (usa setPath/setRoot).
static LocationConfig makeLocation( const std::string & path, const std::string & root ) {
	LocationConfig loc;
	loc.setPath( path );
	loc.setRoot( root );
	return loc;
}

// ADAPTE aqui: unico ponto que depende da assinatura de resolveFilePath.
// Assinatura atual: std::string resolveFilePath( HttpResponse &, const std::string & requestPath, const LocationConfig & )
// Devolve true se nao houve erro na response; o caminho vem em out.
static bool callResolveFilePath( HttpResponse & response,
								 const std::string & requestPath,
								 const LocationConfig & location,
								 std::string & out ) {
	out = resolveFilePath( response, requestPath, location );
	return !response.isError();
}

// devolve o caminho resolvido, ou "<ERROR>" se a funcao sinalizar falha
static std::string filePathOf( const std::string & requestPath,
							   const std::string & locPath,
							   const std::string & root ) {
	LocationConfig loc = makeLocation( locPath, root );
	HttpResponse response;
	std::string out;
	if ( !callResolveFilePath( response, requestPath, loc, out ) )
		return "<ERROR>";
	return out;
}

// ---------- testes: resolveLocation ----------

static void testBasicPrefix() {
	ServerConfig cfg = makeConfig( makePaths3( "/", "/img", "/img/boo" ) );

	check( resolvedPath( "/", cfg ) == "/", "basic: / -> /" );
	check( resolvedPath( "/index.html", cfg ) == "/", "basic: /index.html -> /" );
	check( resolvedPath( "/img", cfg ) == "/img", "basic: /img -> /img (exact)" );
	check( resolvedPath( "/img/x", cfg ) == "/img", "basic: /img/x -> /img" );
	check( resolvedPath( "/img/boo", cfg ) == "/img/boo", "basic: /img/boo -> /img/boo (exact)" );
	check( resolvedPath( "/img/boo/x", cfg ) == "/img/boo", "basic: /img/boo/x -> /img/boo" );
	check( resolvedPath( "/img/boo/ndg/h", cfg ) == "/img/boo", "basic: deep path -> /img/boo" );
}

static void testSegmentBoundary() {
	ServerConfig cfg = makeConfig( makePaths3( "/", "/img", "/img/boo" ) );

	check( resolvedPath( "/imagens", cfg ) == "/", "boundary: /imagens must not match /img" );
	check( resolvedPath( "/img2", cfg ) == "/", "boundary: /img2 must not match /img" );
	check( resolvedPath( "/img/booo", cfg ) == "/img", "boundary: /img/booo must not match /img/boo" );
	check( resolvedPath( "/img/boo2/x", cfg ) == "/img", "boundary: /img/boo2/x -> /img" );
	check( resolvedPath( "/im", cfg ) == "/", "boundary: /im shorter than /img -> /" );
}

static void testLongestWinsOrderIndependent() {
	ServerConfig a = makeConfig( makePaths3( "/img", "/img/boo/ndg", "/" ) );
	ServerConfig b = makeConfig( makePaths3( "/img/boo/ndg", "/", "/img" ) );
	ServerConfig c = makeConfig( makePaths3( "/", "/img", "/img/boo/ndg" ) );

	check( resolvedPath( "/img/boo/ndg/h", a ) == "/img/boo/ndg", "longest: order A" );
	check( resolvedPath( "/img/boo/ndg/h", b ) == "/img/boo/ndg", "longest: order B" );
	check( resolvedPath( "/img/boo/ndg/h", c ) == "/img/boo/ndg", "longest: order C" );
	check( resolvedPath( "/img/boo/ndg/h", a ) == resolvedPath( "/img/boo/ndg/h", c ), "longest: result independent of order" );
}

static void testNoMatch() {
	ServerConfig cfg = makeConfig( makePaths3( "/img", "/css", "/js" ) );

	check( resolvedPath( "/", cfg ) == "<NULL>", "nomatch: / with no root location -> NULL" );
	check( resolvedPath( "/other", cfg ) == "<NULL>", "nomatch: /other -> NULL" );
	check( resolvedPath( "/imgx", cfg ) == "<NULL>", "nomatch: /imgx -> NULL" );
	check( resolvedPath( "/im", cfg ) == "<NULL>", "nomatch: /im -> NULL" );
	check( resolvedPath( "", cfg ) == "<NULL>", "nomatch: empty request path -> NULL" );

	ServerConfig empty = makeConfig( std::vector<std::string>() );
	check( resolvedPath( "/", empty ) == "<NULL>", "nomatch: no locations -> NULL" );
	check( resolvedPath( "/img", empty ) == "<NULL>", "nomatch: no locations, /img -> NULL" );
}

static void testTrailingSlashLocations() {
	ServerConfig cfg = makeConfig( makePaths3( "/", "/img/", "/css/style/" ) );

	check( resolvedPath( "/img/boo", cfg ) == "/img/", "slash-loc: /img/boo -> /img/" );
	check( resolvedPath( "/img/", cfg ) == "/img/", "slash-loc: /img/ exact -> /img/" );
	check( resolvedPath( "/img/a/b", cfg ) == "/img/", "slash-loc: /img/a/b -> /img/" );
	check( resolvedPath( "/imagens/x", cfg ) == "/", "slash-loc: /imagens/x -> /" );
	// path sem a barra final e menor que a location: nao casa (comportamento atual)
	check( resolvedPath( "/img", cfg ) == "/", "slash-loc: /img (no trailing slash) falls to /" );
	check( resolvedPath( "/css/style/a.css", cfg ) == "/css/style/", "slash-loc: nested slash-loc" );
}

static void testRootOnly() {
	ServerConfig cfg = makeConfig( makePaths1( "/" ) );

	check( resolvedPath( "/", cfg ) == "/", "root-only: /" );
	check( resolvedPath( "/a", cfg ) == "/", "root-only: /a" );
	check( resolvedPath( "/a/b/c/d.html", cfg ) == "/", "root-only: deep path" );
	check( resolvedPath( "//", cfg ) == "/", "root-only: // still matches /" );
}

static void testEmptyLocationPathIgnored() {
	std::vector<std::string> v;
	v.push_back( "" );
	v.push_back( "/img" );
	ServerConfig cfg = makeConfig( v );

	check( resolvedPath( "/img/x", cfg ) == "/img", "empty-loc: empty path location ignored" );
	check( resolvedPath( "/other", cfg ) == "<NULL>", "empty-loc: empty location does not act as default" );
}

static void testExactLengthEqual() {
	ServerConfig cfg = makeConfig( makePaths3( "/a", "/ab", "/abc" ) );

	check( resolvedPath( "/a", cfg ) == "/a", "equal-len: /a" );
	check( resolvedPath( "/ab", cfg ) == "/ab", "equal-len: /ab" );
	check( resolvedPath( "/abc", cfg ) == "/abc", "equal-len: /abc" );
	check( resolvedPath( "/abcd", cfg ) == "<NULL>", "equal-len: /abcd -> NULL (no boundary)" );
	check( resolvedPath( "/abc/d", cfg ) == "/abc", "equal-len: /abc/d -> /abc" );
	check( resolvedPath( "/ab/c", cfg ) == "/ab", "equal-len: /ab/c -> /ab" );
}

static void testDuplicateLocationsFirstWins() {
	std::vector<std::string> v;
	v.push_back( "/img" );
	v.push_back( "/img" );
	ServerConfig cfg = makeConfig( v );

	const LocationConfig * got = resolveLocation( "/img/x", cfg );
	check( got == &cfg.getLocations()[0], "duplicate: first declared wins (strict >)" );
}

static void testReturnsPointerIntoConfig() {
	ServerConfig cfg = makeConfig( makePaths3( "/", "/img", "/css" ) );
	const std::vector<LocationConfig> & locs = cfg.getLocations();

	check( resolveLocation( "/img/x", cfg ) == &locs[1], "pointer: points to element inside config (/img)" );
	check( resolveLocation( "/css", cfg ) == &locs[2], "pointer: points to element inside config (/css)" );
	check( resolveLocation( "/zzz", cfg ) == &locs[0], "pointer: falls to element 0 (/)" );
}

static void testCaseSensitive() {
	ServerConfig cfg = makeConfig( makePaths3( "/", "/img", "/Img" ) );

	check( resolvedPath( "/img/a", cfg ) == "/img", "case: /img/a -> /img" );
	check( resolvedPath( "/Img/a", cfg ) == "/Img", "case: /Img/a -> /Img" );
	check( resolvedPath( "/IMG/a", cfg ) == "/", "case: /IMG/a -> / (case-sensitive)" );
}

static void testSpecialCharsInPath() {
	ServerConfig cfg = makeConfig( makePaths3( "/", "/img", "/img/boo" ) );

	check( resolvedPath( "/img/boo/..", cfg ) == "/img/boo", "special: dotdot is not resolved here" );
	check( resolvedPath( "/img/./boo", cfg ) == "/img", "special: dot segment is not normalized here" );
	check( resolvedPath( "/img//boo", cfg ) == "/img", "special: double slash is not normalized here" );
	check( resolvedPath( "/img/boo.png", cfg ) == "/img", "special: /img/boo.png must not match /img/boo" );
	check( resolvedPath( "/img/boo-x", cfg ) == "/img", "special: /img/boo-x must not match /img/boo" );
}

// ---------- testes: resolveFilePath ----------

static void testFilePathSubjectExample() {
	// exemplo literal do subject
	check( filePathOf( "/kapouet/pouic/toto/pouet", "/kapouet", "/tmp/www" ) == "/tmp/www/pouic/toto/pouet",
		"filepath: subject example /kapouet -> /tmp/www" );
	check( filePathOf( "/kapouet/pouic", "/kapouet", "/tmp/www" ) == "/tmp/www/pouic",
		"filepath: one segment after location" );
	check( filePathOf( "/kapouet/a/b/c/d/e", "/kapouet", "/tmp/www" ) == "/tmp/www/a/b/c/d/e",
		"filepath: deep path" );
}

static void testFilePathEqualToLocation() {
	// leftover vazio: resultado e o proprio root
	check( filePathOf( "/kapouet", "/kapouet", "/tmp/www" ) == "/tmp/www",
		"filepath: path equal to location returns root" );
	check( filePathOf( "/", "/", "/tmp/www" ) == "/tmp/www",
		"filepath: / with location / returns root" );
	check( filePathOf( "/img/", "/img/", "/tmp/www" ) == "/tmp/www",
		"filepath: /img/ with location /img/ returns root" );
	// root com barra final e leftover vazio: devolve o root como esta
	check( filePathOf( "/kapouet", "/kapouet", "/tmp/www/" ) == "/tmp/www/",
		"filepath: path equal to location, root with trailing slash returned as is" );
}

static void testFilePathRootTrailingSlash() {
	check( filePathOf( "/kapouet/pouic/toto/pouet", "/kapouet", "/tmp/www/" ) == "/tmp/www/pouic/toto/pouet",
		"filepath: root with trailing slash, no double slash" );
	check( filePathOf( "/kapouet/pouic", "/kapouet", "/tmp/www/" ) == "/tmp/www/pouic",
		"filepath: root with trailing slash, one segment" );
}

static void testFilePathRootWithoutTrailingSlash() {
	check( filePathOf( "/kapouet/pouic", "/kapouet", "/tmp/www" ) == "/tmp/www/pouic",
		"filepath: root without trailing slash, leftover starts with /" );
}

static void testFilePathLocationTrailingSlash() {
	// location termina em '/': leftover nao comeca com '/'
	check( filePathOf( "/index.html", "/", "/tmp/www" ) == "/tmp/www/index.html",
		"filepath: location /, root without slash" );
	check( filePathOf( "/index.html", "/", "/tmp/www/" ) == "/tmp/www/index.html",
		"filepath: location /, root with slash" );
	check( filePathOf( "/img/boo", "/img/", "/tmp/www" ) == "/tmp/www/boo",
		"filepath: location /img/, root without slash" );
	check( filePathOf( "/img/boo", "/img/", "/tmp/www/" ) == "/tmp/www/boo",
		"filepath: location /img/, root with slash" );
	check( filePathOf( "/img/a/b/c.png", "/img/", "/tmp/www" ) == "/tmp/www/a/b/c.png",
		"filepath: location /img/, nested file" );
}

static void testFilePathRootIsSlash() {
	// root que e so "/" nao pode ser esvaziado
	check( filePathOf( "/kapouet/pouic", "/kapouet", "/" ) == "/pouic",
		"filepath: root /, leftover starts with /" );
	check( filePathOf( "/a", "/", "/" ) == "/a",
		"filepath: root / and location /" );
	check( filePathOf( "/kapouet", "/kapouet", "/" ) == "/",
		"filepath: root / with leftover empty returns /" );
}

static void testFilePathRelativeRoot() {
	check( filePathOf( "/kapouet/pouic", "/kapouet", "www" ) == "www/pouic",
		"filepath: relative root www" );
	check( filePathOf( "/kapouet/pouic", "/kapouet", "./www" ) == "./www/pouic",
		"filepath: relative root ./www" );
	check( filePathOf( "/kapouet/pouic", "/kapouet", "./www/" ) == "./www/pouic",
		"filepath: relative root ./www/ (trailing slash)" );
	check( filePathOf( "/kapouet", "/kapouet", "www" ) == "www",
		"filepath: relative root, leftover empty" );
}

static void testFilePathTrailingSlashInRequest() {
	// barra final na request e preservada (importante para distinguir diretorio)
	check( filePathOf( "/kapouet/pouic/", "/kapouet", "/tmp/www" ) == "/tmp/www/pouic/",
		"filepath: trailing slash in request preserved" );
	check( filePathOf( "/kapouet/", "/kapouet", "/tmp/www" ) == "/tmp/www/",
		"filepath: /kapouet/ with location /kapouet -> root/" );
	check( filePathOf( "/kapouet/", "/kapouet", "/tmp/www/" ) == "/tmp/www/",
		"filepath: /kapouet/ with root slash -> no double slash" );
}

static void testFilePathEmptyRoot() {
	LocationConfig loc = makeLocation( "/kapouet", "" );

	{
		HttpResponse response;
		std::string out;
		bool ok = callResolveFilePath( response, "/kapouet/pouic", loc, out );
		check( !ok, "filepath: empty root fails (leftover not empty)" );
		check( response.getStatusCode() == STATUS_INTERNAL_SERVER_ERROR,
			"filepath: empty root sets 500 (leftover not empty)" );
		check( out.empty(), "filepath: empty root returns empty string (leftover not empty)" );
	}
	{
		HttpResponse response;
		std::string out;
		bool ok = callResolveFilePath( response, "/kapouet", loc, out );
		check( !ok, "filepath: empty root fails (leftover empty)" );
		check( response.getStatusCode() == STATUS_INTERNAL_SERVER_ERROR,
			"filepath: empty root sets 500 (leftover empty)" );
		check( out.empty(), "filepath: empty root returns empty string (leftover empty)" );
	}
}

static void testFilePathSuccessLeavesResponseUntouched() {
	LocationConfig loc = makeLocation( "/kapouet", "/tmp/www" );
	HttpResponse response;
	std::string out;

	bool ok = callResolveFilePath( response, "/kapouet/pouic", loc, out );
	check( ok, "filepath: valid root does not flag an error" );
	check( !response.isError(), "filepath: response not an error after success" );
}

static void testFilePathSpecialNames() {
	check( filePathOf( "/kapouet/file.tar.gz", "/kapouet", "/tmp/www" ) == "/tmp/www/file.tar.gz",
		"filepath: dots in filename" );
	check( filePathOf( "/kapouet/my file.txt", "/kapouet", "/tmp/www" ) == "/tmp/www/my file.txt",
		"filepath: space in filename (already decoded)" );
	check( filePathOf( "/kapouet/.hidden", "/kapouet", "/tmp/www" ) == "/tmp/www/.hidden",
		"filepath: hidden file" );
}

static void testFilePathLocationDifferentFromRoot() {
	// o nome da location nao aparece no caminho final
	check( filePathOf( "/img/boo/x.png", "/img", "/var/assets" ) == "/var/assets/boo/x.png",
		"filepath: location name is replaced by root" );
	check( filePathOf( "/img/boo/x.png", "/img/boo", "/var/assets" ) == "/var/assets/x.png",
		"filepath: nested location name is replaced by root" );
}

void runResolveLocationTests() {
	RUN( testBasicPrefix );
	RUN( testSegmentBoundary );
	RUN( testLongestWinsOrderIndependent );
	RUN( testNoMatch );
	RUN( testTrailingSlashLocations );
	RUN( testRootOnly );
	RUN( testEmptyLocationPathIgnored );
	RUN( testExactLengthEqual );
	RUN( testDuplicateLocationsFirstWins );
	RUN( testReturnsPointerIntoConfig );
	RUN( testCaseSensitive );
	RUN( testSpecialCharsInPath );
}

void runResolveFilePathTests() {
	RUN( testFilePathSubjectExample );
	RUN( testFilePathEqualToLocation );
	RUN( testFilePathRootTrailingSlash );
	RUN( testFilePathRootWithoutTrailingSlash );
	RUN( testFilePathLocationTrailingSlash );
	RUN( testFilePathRootIsSlash );
	RUN( testFilePathRelativeRoot );
	RUN( testFilePathTrailingSlashInRequest );
	RUN( testFilePathEmptyRoot );
	RUN( testFilePathSuccessLeavesResponseUntouched );
	RUN( testFilePathSpecialNames );
	RUN( testFilePathLocationDifferentFromRoot );
}