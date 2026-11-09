#include <string>
#include <vector>

#include "config/LocationConfig.hpp"
#include "config/ServerConfig.hpp"
#include "../tests.hpp"
#include "HttpRequest/RequestProcessor.cpp"

// Pre-condicoes garantidas pelo config (nao testadas aqui):
//  - path de location nao vazio e comecando com '/'
//  - sem paths duplicados (o teste de duplicata so documenta o desempate)

// --------------------------------------------------
// Helpers
// --------------------------------------------------

static ServerConfig makeConfig( const std::vector<std::string> & paths ) {
	ServerConfig config;
	for ( size_t i = 0; i < paths.size(); i++ ) {
		LocationConfig loc;
		loc.setPath( paths[i] );
		config.addLocation( loc );
	}
	return config;
}

static std::vector<std::string> paths( const char * a = NULL,
									   const char * b = NULL,
									   const char * c = NULL,
									   const char * d = NULL ) {
	std::vector<std::string> v;
	if ( a ) v.push_back( a );
	if ( b ) v.push_back( b );
	if ( c ) v.push_back( c );
	if ( d ) v.push_back( d );
	return v;
}

// path da location escolhida, ou "<NULL>"
static std::string resolved( const std::string & request, const ServerConfig & config ) {
	const LocationConfig * loc = resolveLocation( request, config );
	return loc ? loc->getPath() : std::string( "<NULL>" );
}

// --------------------------------------------------
// Casos basicos
// --------------------------------------------------

static void testBasicPrefix() {
	ServerConfig cfg = makeConfig( paths( "/", "/img", "/img/boo" ) );

	check( resolved( "/", cfg ) == "/",                         "basic: / -> /" );
	check( resolved( "/index.html", cfg ) == "/",               "basic: /index.html -> /" );
	check( resolved( "/img", cfg ) == "/img",                   "basic: /img exact" );
	check( resolved( "/img/x", cfg ) == "/img",                 "basic: /img/x -> /img" );
	check( resolved( "/img/boo", cfg ) == "/img/boo",           "basic: /img/boo exact" );
	check( resolved( "/img/boo/x", cfg ) == "/img/boo",         "basic: /img/boo/x -> /img/boo" );
	check( resolved( "/img/boo/a/b/c/d", cfg ) == "/img/boo",   "basic: deep path -> /img/boo" );
}

static void testRootOnly() {
	ServerConfig cfg = makeConfig( paths( "/" ) );

	check( resolved( "/", cfg ) == "/",                  "root-only: /" );
	check( resolved( "/a", cfg ) == "/",                 "root-only: /a" );
	check( resolved( "/a/b/c/d.html", cfg ) == "/",      "root-only: deep path" );
	check( resolved( "//", cfg ) == "/",                 "root-only: // still matches /" );
}

// --------------------------------------------------
// Fronteira de segmento
// --------------------------------------------------

static void testSegmentBoundary() {
	ServerConfig cfg = makeConfig( paths( "/", "/img", "/img/boo" ) );

	check( resolved( "/imagens", cfg ) == "/",       "boundary: /imagens must not match /img" );
	check( resolved( "/img2", cfg ) == "/",          "boundary: /img2 must not match /img" );
	check( resolved( "/imgx/a", cfg ) == "/",        "boundary: /imgx/a must not match /img" );
	check( resolved( "/img/booo", cfg ) == "/img",   "boundary: /img/booo must not match /img/boo" );
	check( resolved( "/img/boo2/x", cfg ) == "/img", "boundary: /img/boo2/x -> /img" );
	check( resolved( "/img/boo.png", cfg ) == "/img","boundary: /img/boo.png must not match /img/boo" );
	check( resolved( "/img/boo-x", cfg ) == "/img",  "boundary: /img/boo-x must not match /img/boo" );
	check( resolved( "/im", cfg ) == "/",            "boundary: /im shorter than /img -> /" );
}

static void testExactLengthEdges() {
	ServerConfig cfg = makeConfig( paths( "/a", "/ab", "/abc" ) );

	check( resolved( "/a", cfg ) == "/a",            "equal-len: /a" );
	check( resolved( "/ab", cfg ) == "/ab",          "equal-len: /ab" );
	check( resolved( "/abc", cfg ) == "/abc",        "equal-len: /abc" );
	check( resolved( "/abcd", cfg ) == "<NULL>",     "equal-len: /abcd has no boundary -> NULL" );
	check( resolved( "/abc/d", cfg ) == "/abc",      "equal-len: /abc/d -> /abc" );
	check( resolved( "/ab/c", cfg ) == "/ab",        "equal-len: /ab/c -> /ab" );
	check( resolved( "/a/b", cfg ) == "/a",          "equal-len: /a/b -> /a" );
}

// --------------------------------------------------
// Mais longo vence
// --------------------------------------------------

static void testLongestWins() {
	ServerConfig cfg = makeConfig( paths( "/", "/img", "/img/boo", "/img/boo/ndg" ) );

	check( resolved( "/img/boo/ndg/h", cfg ) == "/img/boo/ndg", "longest: deepest wins" );
	check( resolved( "/img/boo/ndgx", cfg ) == "/img/boo",      "longest: partial last segment falls back" );
	check( resolved( "/img/boo/x", cfg ) == "/img/boo",         "longest: middle level" );
}

static void testLongestIsOrderIndependent() {
	const char * p[3] = { "/", "/img", "/img/boo" };
	const int perms[6][3] = {
		{0,1,2}, {0,2,1}, {1,0,2}, {1,2,0}, {2,0,1}, {2,1,0}
	};
	const char * requests[5] = { "/", "/x", "/img", "/img/boo", "/img/boo/z" };
	const char * expected[5] = { "/", "/", "/img", "/img/boo", "/img/boo" };

	bool allOk = true;
	for ( int i = 0; i < 6; i++ ) {
		ServerConfig cfg = makeConfig( paths( p[perms[i][0]], p[perms[i][1]], p[perms[i][2]] ) );
		for ( int r = 0; r < 5; r++ ) {
			if ( resolved( requests[r], cfg ) != expected[r] )
				allOk = false;
		}
	}
	check( allOk, "order: all 6 declaration orders give the same result" );
}

// --------------------------------------------------
// Locations com barra final
// --------------------------------------------------

static void testTrailingSlashLocations() {
	ServerConfig cfg = makeConfig( paths( "/", "/img/", "/css/style/" ) );

	check( resolved( "/img/", cfg ) == "/img/",                  "slash-loc: /img/ exact" );
	check( resolved( "/img/boo", cfg ) == "/img/",               "slash-loc: /img/boo -> /img/" );
	check( resolved( "/img/a/b", cfg ) == "/img/",               "slash-loc: /img/a/b -> /img/" );
	check( resolved( "/img", cfg ) == "/",                       "slash-loc: /img (no slash) falls to /" );
	check( resolved( "/imagens/x", cfg ) == "/",                 "slash-loc: /imagens/x -> /" );
	check( resolved( "/css/style/a.css", cfg ) == "/css/style/", "slash-loc: nested slash location" );
	check( resolved( "/css/style", cfg ) == "/",                 "slash-loc: nested without slash -> /" );
	check( resolved( "/css/", cfg ) == "/",                      "slash-loc: parent of nested -> /" );
}

static void testSlashAndNoSlashCoexist() {
	ServerConfig cfg = makeConfig( paths( "/", "/img", "/img/" ) );

	check( resolved( "/img", cfg ) == "/img",    "coexist: /img exact -> /img" );
	check( resolved( "/img/", cfg ) == "/img/",  "coexist: /img/ -> /img/ (longer wins)" );
	check( resolved( "/img/x", cfg ) == "/img/", "coexist: /img/x -> /img/ (longer wins)" );
	check( resolved( "/imgx", cfg ) == "/",      "coexist: /imgx -> /" );
}

// --------------------------------------------------
// Sem correspondencia
// --------------------------------------------------

static void testNoMatch() {
	ServerConfig cfg = makeConfig( paths( "/img", "/css", "/js" ) );

	check( resolved( "/", cfg ) == "<NULL>",       "nomatch: / without root location -> NULL" );
	check( resolved( "/other", cfg ) == "<NULL>",  "nomatch: /other -> NULL" );
	check( resolved( "/imgx", cfg ) == "<NULL>",   "nomatch: /imgx -> NULL" );
	check( resolved( "/im", cfg ) == "<NULL>",     "nomatch: /im -> NULL" );
	check( resolved( "/jss", cfg ) == "<NULL>",    "nomatch: /jss -> NULL" );

	ServerConfig empty = makeConfig( paths() );
	check( resolved( "/", empty ) == "<NULL>",     "nomatch: no locations, / -> NULL" );
	check( resolved( "/img", empty ) == "<NULL>",  "nomatch: no locations, /img -> NULL" );
}

static void testMalformedRequestPaths() {
	ServerConfig cfg = makeConfig( paths( "/", "/img" ) );

	check( resolved( "", cfg ) == "<NULL>",            "malformed: empty request path -> NULL" );
	check( resolved( "index.html", cfg ) == "<NULL>",  "malformed: no leading slash -> NULL" );
	check( resolved( "img/a", cfg ) == "<NULL>",       "malformed: img/a (no leading slash) -> NULL" );
}

// --------------------------------------------------
// Sensibilidade a maiusculas e caracteres especiais
// --------------------------------------------------

static void testCaseSensitive() {
	ServerConfig cfg = makeConfig( paths( "/", "/img", "/Img" ) );

	check( resolved( "/img/a", cfg ) == "/img", "case: /img/a -> /img" );
	check( resolved( "/Img/a", cfg ) == "/Img", "case: /Img/a -> /Img" );
	check( resolved( "/IMG/a", cfg ) == "/",    "case: /IMG/a -> / (case-sensitive)" );
}

static void testNoNormalizationHere() {
	ServerConfig cfg = makeConfig( paths( "/", "/img", "/img/boo" ) );

	// o parser ja rejeitou '..'; aqui nada e normalizado
	check( resolved( "/img/boo/..", cfg ) == "/img/boo",  "raw: '..' segment is not resolved here" );
	check( resolved( "/img/./boo", cfg ) == "/img",       "raw: '.' segment is not normalized" );
	check( resolved( "/img//boo", cfg ) == "/img",        "raw: '//' is not collapsed" );
	check( resolved( "/img%2Fboo", cfg ) == "/",          "raw: '%2F' is not decoded here" );
	check( resolved( "/img boo", cfg ) == "/",            "raw: space is not a boundary" );
	check( resolved( "/img.", cfg ) == "/",               "raw: '.' after name is not a boundary" );
}

static void testLocationNamesWithSpecialChars() {
	ServerConfig cfg = makeConfig( paths( "/", "/my-files", "/v1.0", "/a_b" ) );

	check( resolved( "/my-files/x", cfg ) == "/my-files", "chars: dash in location" );
	check( resolved( "/v1.0/api", cfg ) == "/v1.0",       "chars: dot in location" );
	check( resolved( "/v1x0/api", cfg ) == "/",           "chars: dot is literal, not a wildcard" );
	check( resolved( "/a_b/c", cfg ) == "/a_b",           "chars: underscore in location" );
}

// --------------------------------------------------
// Ponteiro, duplicatas, entradas grandes
// --------------------------------------------------

static void testReturnsPointerIntoConfig() {
	ServerConfig cfg = makeConfig( paths( "/", "/img", "/css" ) );
	const std::vector<LocationConfig> & locs = cfg.getLocations();

	check( resolveLocation( "/img/x", cfg ) == &locs[1], "pointer: /img points into config" );
	check( resolveLocation( "/css", cfg ) == &locs[2],   "pointer: /css points into config" );
	check( resolveLocation( "/zzz", cfg ) == &locs[0],   "pointer: fallback points to /" );
}

static void testDuplicateFirstWins() {
	// o config deve rejeitar duplicatas; isto so documenta o desempate
	ServerConfig cfg = makeConfig( paths( "/img", "/img" ) );

	check( resolveLocation( "/img/x", cfg ) == &cfg.getLocations()[0],
		   "duplicate: first declared wins (strict >)" );
}

static void testVeryLongPath() {
	ServerConfig cfg = makeConfig( paths( "/", "/img" ) );

	std::string longPath = "/img/" + std::string( 10000, 'a' );
	check( resolved( longPath, cfg ) == "/img",              "long: very long path under /img" );

	std::string longNoMatch = "/" + std::string( 10000, 'b' );
	check( resolved( longNoMatch, cfg ) == "/",              "long: very long path falls to /" );
}

static void testLongLocationPath() {
	std::string deep;
	for ( int i = 0; i < 50; i++ )
		deep += "/seg";

	ServerConfig cfg = makeConfig( paths( "/", deep.c_str() ) );

	check( resolved( deep, cfg ) == deep,                    "long-loc: exact deep location" );
	check( resolved( deep + "/file", cfg ) == deep,          "long-loc: file under deep location" );
	check( resolved( deep + "x", cfg ) == "/",               "long-loc: no boundary after deep location" );
}

// --------------------------------------------------
// Runner
// --------------------------------------------------

void runResolveLocationTests() {
	RUN( testBasicPrefix );
	RUN( testRootOnly );
	RUN( testSegmentBoundary );
	RUN( testExactLengthEdges );
	RUN( testLongestWins );
	RUN( testLongestIsOrderIndependent );
	RUN( testTrailingSlashLocations );
	RUN( testSlashAndNoSlashCoexist );
	RUN( testNoMatch );
	RUN( testMalformedRequestPaths );
	RUN( testCaseSensitive );
	RUN( testNoNormalizationHere );
	RUN( testLocationNamesWithSpecialChars );
	RUN( testReturnsPointerIntoConfig );
	RUN( testDuplicateFirstWins );
	RUN( testVeryLongPath );
	RUN( testLongLocationPath );
}