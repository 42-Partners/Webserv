#include "HttpRequest/HttpRequest.hpp"
#include "StatusCode/StatusCode.hpp"
#include "../tests.hpp"

#include <iostream>
#include <sstream>
#include <vector>

// ===========================================================================
// Limites esperados. Mantenha em sincronia com HttpRequest.hpp / config.
// kHuge precisa ficar bem acima de MAX_CHUNK_SIZE_LINE.
// ===========================================================================

const size_t kMaxRequestLine = 8192;
const size_t kMaxHeadersSize = 8192;
const size_t kMaxHeaderCount = 100;
const size_t kDefaultMaxBody = 1048576;
const size_t kHuge           = 10000;

// Prefixos reutilizados (literais, para concatenar com outros literais).
#define REQLINE "GET / HTTP/1.1\r\n"
#define HOST    "Host: localhost\r\n"
#define GETH    REQLINE HOST
#define CHUNKED "POST /upload HTTP/1.1\r\n" HOST "Transfer-Encoding: chunked\r\n\r\n"
#define RL(uri) "GET " uri " HTTP/1.1\r\n"

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

// ===========================================================================
// Infraestrutura
// ===========================================================================

int g_passed = 0;
int g_failed = 0;
bool g_verbose = false;

void check(bool ok, const std::string& name) {
	if (ok) {
		++g_passed;
		if (g_verbose)
			std::cout << "[PASS] " << name << "\n";
	} else {
		++g_failed;
		std::cout << "[FAIL] " << name << "\n";
	}
}

const char* stateName(int s) {
	if (s == HttpRequest::PARSING_REQUEST_LINE) return "PARSING_REQUEST_LINE";
	if (s == HttpRequest::PARSING_HEADERS)      return "PARSING_HEADERS";
	if (s == HttpRequest::PARSING_BODY)         return "PARSING_BODY";
	if (s == HttpRequest::CHUNK_TRAILERS)       return "CHUNK_TRAILERS";
	if (s == HttpRequest::REQUEST_COMPLETE)     return "REQUEST_COMPLETE";
	if (s == HttpRequest::REQUEST_ERROR)        return "REQUEST_ERROR";
	return "?";
}

std::string describe(const HttpRequest& r) {
	std::ostringstream oss;
	oss << "estado=" << stateName(r.getState()) << " codigo=" << r.getErrorCode();
	return oss.str();
}

void expectState(const HttpRequest& r, int state, const std::string& name) {
	bool ok = (r.getState() == state && r.getErrorCode() == 0);
	check(ok, ok ? name : name + " [" + describe(r) + "]");
}

std::string num(size_t n) {
	std::ostringstream oss;
	oss << n;
	return oss.str();
}

void expectError(const HttpRequest& r, int code, const std::string& name) {
	bool ok = (r.getState() == HttpRequest::REQUEST_ERROR && r.getErrorCode() == code);
	check(ok, ok ? name : name + " [esperado erro " + num(code) + ", obtido " + describe(r) + "]");
}

void expectComplete(const HttpRequest& r, const std::string& name) {
	expectState(r, HttpRequest::REQUEST_COMPLETE, name);
}

bool isDone(const HttpRequest& r) {
	return r.getState() == HttpRequest::REQUEST_COMPLETE ||
	       r.getState() == HttpRequest::REQUEST_ERROR;
}

void feedAll(HttpRequest& r, const std::string& data) {
	std::string buffer(data);
	r.feed(buffer);
}

// Alimenta byte a byte e para ao chegar em um estado terminal.
void feedByByte(HttpRequest& r, const std::string& data) {
	std::string buffer;
	for (size_t i = 0; i < data.size() && !isDone(r); ++i) {
		buffer += data[i];
		r.feed(buffer);
	}
}

// Alimenta byte a byte. Devolve false se o estado virou COMPLETE ou ERROR
// antes do ultimo byte.
bool feedByteByByteStrict(HttpRequest& r, const std::string& data) {
	std::string buffer;
	for (size_t i = 0; i < data.size(); ++i) {
		buffer += data[i];
		r.feed(buffer);
		if (i + 1 != data.size() && isDone(r))
			return false;
	}
	return true;
}

std::string lengthHeaders(size_t length) {
	return "POST /upload HTTP/1.1\r\n" HOST "Content-Length: " + num(length) + "\r\n\r\n";
}

std::string chunkedHeaders() {
	return CHUNKED;
}

// 413 em plataformas 64 bits (F*16 cabe em size_t); 400 se houver overflow.
int hugeChunkCode() {
	return sizeof(size_t) >= 8 ? STATUS_CONTENT_TOO_LARGE : STATUS_BAD_REQUEST;
}

// ---------------------------------------------------------------------------
// Equivalencia entre alimentar tudo de uma vez e alimentar em fragmentos.
// ---------------------------------------------------------------------------

struct Snapshot {
	int         state;
	int         code;
	std::string body;
	std::string path;
	std::string query;
	std::string leftover;
};

Snapshot snapshot(const HttpRequest& r, const std::string& leftover) {
	Snapshot s;
	s.state    = r.getState();
	s.code     = r.getErrorCode();
	s.body     = r.getBody();
	s.path     = r.getPath();
	s.query    = r.getQueryString();
	s.leftover = leftover;
	return s;
}

// body, path e sobra so sao comparados quando a requisicao completou.
bool sameSnapshot(const Snapshot& a, const Snapshot& b) {
	if (a.state != b.state || a.code != b.code)
		return false;
	if (a.state == HttpRequest::REQUEST_COMPLETE)
		return a.body == b.body && a.path == b.path &&
		       a.query == b.query && a.leftover == b.leftover;
	return true;
}

Snapshot wholeSnapshot(const std::string& input, size_t maxBody) {
	HttpRequest r(maxBody);
	std::string buffer(input);
	r.feed(buffer);
	return snapshot(r, buffer);
}

unsigned nextRandom(unsigned& seed) {
	seed = seed * 1103515245u + 12345u;
	return (seed >> 16) & 0x7fff;
}

// Corta a entrada em todos os pontos possiveis (2 pedacos) e tambem byte a
// byte; o resultado precisa ser igual ao de alimentar tudo de uma vez.
void checkSplitEquivalence(const std::string& name, const std::string& input,
                                  size_t maxBody = kDefaultMaxBody) {
	Snapshot expected = wholeSnapshot(input, maxBody);

	if (input.size() <= 600) {
		bool   ok = true;
		size_t badCut = 0;
		for (size_t k = 1; k < input.size() && ok; ++k) {
			HttpRequest r(maxBody);
			std::string buffer(input, 0, k);
			r.feed(buffer);
			buffer.append(input, k, std::string::npos);
			r.feed(buffer);
			if (!sameSnapshot(expected, snapshot(r, buffer))) {
				ok = false;
				badCut = k;
			}
		}
		check(ok, ok ? name + " [mesmo resultado em qualquer corte]"
		             : name + " [resultado diferente ao cortar no byte " + num(badCut) + "]");
	}

	if (input.size() <= 4096) {
		HttpRequest r(maxBody);
		std::string buffer;
		for (size_t i = 0; i < input.size(); ++i) {
			buffer += input[i];
			r.feed(buffer);
		}
		check(sameSnapshot(expected, snapshot(r, buffer)),
		      name + " [mesmo resultado byte a byte]");
	}
}

// Fragmentacao pseudo-aleatoria (reproduzivel) com pedacos de 1 a 17 bytes.
void checkRandomSplits(const std::string& name, const std::string& input,
                              size_t maxBody = kDefaultMaxBody, int rounds = 200) {
	Snapshot expected = wholeSnapshot(input, maxBody);
	bool ok = true;
	int  badRound = -1;

	for (int round = 0; round < rounds && ok; ++round) {
		unsigned seed = static_cast<unsigned>(round) * 2654435761u + 1u;
		HttpRequest r(maxBody);
		std::string buffer;
		size_t pos = 0;
		while (pos < input.size()) {
			size_t piece = 1 + nextRandom(seed) % 17;
			buffer.append(input, pos, piece);
			pos += piece;
			r.feed(buffer);
		}
		if (!sameSnapshot(expected, snapshot(r, buffer))) {
			ok = false;
			badRound = round;
		}
	}
	std::ostringstream oss;
	oss << name << " [fragmentacao aleatoria";
	if (!ok)
		oss << ", falhou na rodada " << badRound;
	oss << "]";
	check(ok, oss.str());
}

// ---------------------------------------------------------------------------
// Tabelas
// ---------------------------------------------------------------------------

struct ErrCase {
	const char* name;
	std::string input;
	int         code;
};

// Cada caso roda: tudo de uma vez, byte a byte e com cortes em qualquer ponto.
void runErrCases(const std::string& group, const ErrCase* cases, size_t n,
                        size_t maxBody = kDefaultMaxBody) {
	for (size_t i = 0; i < n; ++i) {
		std::string name = group + ": " + cases[i].name;

		HttpRequest whole(maxBody);
		feedAll(whole, cases[i].input);
		expectError(whole, cases[i].code, name);

		if (cases[i].input.size() <= 4096) {
			HttpRequest bytes(maxBody);
			feedByByte(bytes, cases[i].input);
			expectError(bytes, cases[i].code, name + " [byte a byte]");
		}
		checkSplitEquivalence(name, cases[i].input, maxBody);
	}
}

// ===========================================================================
// Estado inicial
// ===========================================================================

void testInitialState() {
	HttpRequest r;

	check(r.getState() == HttpRequest::PARSING_REQUEST_LINE, "Inicial: PARSING_REQUEST_LINE");
	check(r.getErrorCode() == 0,    "Inicial: sem erro");
	check(r.getBody().empty(),      "Inicial: body vazio");
	check(r.getKeepAlive() == true, "Inicial: keep-alive habilitado");

	HttpRequest r2;
	std::string empty;
	r2.feed(empty);
	check(r2.getState() == HttpRequest::PARSING_REQUEST_LINE && r2.getErrorCode() == 0,
	      "Buffer vazio: continua aguardando, sem erro");
}

// ===========================================================================
// Request line
// ===========================================================================

void testValidRequest() {
	HttpRequest request;
	std::string buffer = "GET /index.html HTTP/1.1\r\n";

	request.feed(buffer);

	check(request.getState() == HttpRequest::PARSING_HEADERS, "GET valido: estado PARSING_HEADERS");
	check(request.getErrorCode() == 0,                        "GET valido: sem erro");
	check(request.getMethod() == "GET",                       "GET valido: metodo");
	check(request.getUri() == "/index.html",                  "GET valido: URI");
	check(request.getPath() == "/index.html",                 "GET valido: path");
	check(request.getHttpVersion() == "HTTP/1.1",             "GET valido: versao HTTP");
	check(buffer.empty(),                                     "GET valido: buffer consumido");
}

void testSupportedMethods() {
	const char* methods[] = {"GET", "POST", "DELETE"};

	for (size_t i = 0; i < COUNT(methods); ++i) {
		HttpRequest request;
		std::string buffer = std::string(methods[i]) + " / HTTP/1.1\r\n";

		request.feed(buffer);

		check(request.getState() == HttpRequest::PARSING_HEADERS,
		      std::string("Metodo suportado: ") + methods[i]);
		check(request.getMethod() == methods[i],
		      std::string("Metodo armazenado: ") + methods[i]);
	}
}

void testRequestLineErrors() {
	const ErrCase cases[] = {
		{ "espacos extras",                     "GET  /index.html HTTP/1.1\r\n",  STATUS_BAD_REQUEST },
		{ "espaco no fim da linha",             "GET / HTTP/1.1 \r\n",            STATUS_BAD_REQUEST },
		{ "espaco no inicio da linha",          " GET / HTTP/1.1\r\n",            STATUS_BAD_REQUEST },
		{ "TAB como separador",                 "GET\t/\tHTTP/1.1\r\n",           STATUS_BAD_REQUEST },
		{ "so duas partes",                     "GET /\r\n",                      STATUS_BAD_REQUEST },
		{ "so uma parte",                       "GET\r\n",                        STATUS_BAD_REQUEST },
		{ "quatro partes",                      "GET / HTTP/1.1 extra\r\n",       STATUS_BAD_REQUEST },
		{ "URI vazia",                          "GET  HTTP/1.1\r\n",              STATUS_BAD_REQUEST },
		{ "CR solto no fim da linha",           "GET / HTTP/1.1\r\r\n",           STATUS_BAD_REQUEST },

		{ "metodo desconhecido",                "FOO / HTTP/1.1\r\n",             STATUS_NOT_IMPLEMENTED },
		{ "metodo em minusculas",               "get / HTTP/1.1\r\n",             STATUS_NOT_IMPLEMENTED },
		{ "metodo misto",                       "Get / HTTP/1.1\r\n",             STATUS_NOT_IMPLEMENTED },
		{ "HEAD nao suportado",                 "HEAD / HTTP/1.1\r\n",            STATUS_NOT_IMPLEMENTED },
		{ "PUT nao suportado",                  "PUT / HTTP/1.1\r\n",             STATUS_NOT_IMPLEMENTED },
		{ "OPTIONS nao suportado",              "OPTIONS / HTTP/1.1\r\n",         STATUS_NOT_IMPLEMENTED },
		{ "CONNECT nao suportado",              "CONNECT localhost:80 HTTP/1.1\r\n", STATUS_NOT_IMPLEMENTED },
		{ "metodo com tchar valido e longo",    "PROPFIND / HTTP/1.1\r\n",        STATUS_NOT_IMPLEMENTED },
		{ "metodo com caractere invalido @",    "GE@T / HTTP/1.1\r\n",            STATUS_BAD_REQUEST },
		{ "metodo com caractere invalido (",    "GE(T / HTTP/1.1\r\n",            STATUS_BAD_REQUEST },
		{ "metodo com byte alto",               "G\303\251T / HTTP/1.1\r\n",      STATUS_BAD_REQUEST },

		{ "URI sem barra inicial",              "GET index.html HTTP/1.1\r\n",    STATUS_BAD_REQUEST },
		{ "URI asterisco",                      "GET * HTTP/1.1\r\n",             STATUS_BAD_REQUEST },
		{ "URI em forma absoluta",              "GET http://localhost/ HTTP/1.1\r\n", STATUS_BAD_REQUEST },
	};
	runErrCases("Request line", cases, COUNT(cases));
}

void testUriErrors() {
	const ErrCase cases[] = {
		{ "escape hexa invalido %GG",           RL("/%GG"),                       STATUS_BAD_REQUEST },
		{ "escape hexa invalido %G1",           RL("/%G1"),                       STATUS_BAD_REQUEST },
		{ "escape hexa invalido %1G",           RL("/%1G"),                       STATUS_BAD_REQUEST },
		{ "% no fim",                           RL("/a%"),                        STATUS_BAD_REQUEST },
		{ "%4 truncado no fim",                 RL("/a%4"),                       STATUS_BAD_REQUEST },
		{ "% sozinho depois da barra",          RL("/%"),                         STATUS_BAD_REQUEST },
		{ "%00 (NUL codificado)",               RL("/a%00b"),                     STATUS_BAD_REQUEST },
		{ "%0d%0a (CRLF codificado)",           RL("/a%0d%0ab"),                  STATUS_BAD_REQUEST },
		{ "%09 (TAB codificado)",               RL("/a%09b"),                     STATUS_BAD_REQUEST },
		{ "%1f (controle codificado)",          RL("/a%1fb"),                     STATUS_BAD_REQUEST },
		{ "%7f (DEL codificado)",               RL("/a%7fb"),                     STATUS_BAD_REQUEST },
		{ "%80 (byte alto codificado)",         RL("/a%80b"),                     STATUS_BAD_REQUEST },
		{ "%ff (byte alto codificado)",         RL("/a%ffb"),                     STATUS_BAD_REQUEST },
		{ "TAB cru na URI",                     RL("/a\tb"),                      STATUS_BAD_REQUEST },
		{ "controle cru na URI",                RL("/a\001b"),                    STATUS_BAD_REQUEST },
		{ "DEL cru na URI",                     RL("/a\177b"),                    STATUS_BAD_REQUEST },
		{ "byte alto cru na URI (UTF-8)",       RL("/\303\251"),                  STATUS_BAD_REQUEST },

		{ "query com escape invalido %zz",      RL("/a?q=%zz"),                   STATUS_BAD_REQUEST },
		{ "query com % truncado",               RL("/a?q=%4"),                    STATUS_BAD_REQUEST },
		{ "query com % no fim",                 RL("/a?q=%"),                     STATUS_BAD_REQUEST },
		{ "query com controle cru",             RL("/a?q=\001"),                  STATUS_BAD_REQUEST },
		{ "query com byte alto cru",            RL("/a?q=\303\251"),              STATUS_BAD_REQUEST },

		{ "path iniciado com //",               RL("//evil.example/path"),        STATUS_BAD_REQUEST },
		{ "path so com //",                     RL("//"),                         STATUS_BAD_REQUEST },
		{ "path iniciado com ///",              RL("///etc/passwd"),              STATUS_BAD_REQUEST },
		{ "path iniciado com /\\",              RL("/\\evil.example/path"),       STATUS_BAD_REQUEST },
		{ "// via %2f",                         RL("/%2fevil"),                   STATUS_BAD_REQUEST },
		{ "/\\ via %5c",                        RL("/%5cevil"),                   STATUS_BAD_REQUEST },

		{ "traversal simples",                  RL("/../secret"),                 STATUS_FORBIDDEN },
		{ "traversal no meio",                  RL("/a/../b"),                    STATUS_FORBIDDEN },
		{ "traversal no fim",                   RL("/a/.."),                      STATUS_FORBIDDEN },
		{ "traversal so '..'",                  RL("/.."),                        STATUS_FORBIDDEN },
		{ "traversal profundo",                 RL("/a/b/../../../etc/passwd"),   STATUS_FORBIDDEN },
		{ "traversal %2e%2e",                   RL("/%2e%2e/secret"),             STATUS_FORBIDDEN },
		{ "traversal %2E%2E (maiusculo)",       RL("/%2E%2E/secret"),             STATUS_FORBIDDEN },
		{ "traversal .%2e",                     RL("/.%2e/secret"),               STATUS_FORBIDDEN },
		{ "traversal %2e.",                     RL("/%2e./secret"),               STATUS_FORBIDDEN },
		{ "traversal com barra codificada",     RL("/..%2fsecret"),               STATUS_FORBIDDEN },
		{ "traversal %2e%2e%2f",                RL("/%2e%2e%2fsecret"),           STATUS_FORBIDDEN },
		{ "traversal barra no meio codificada", RL("/a%2f..%2fb"),                STATUS_FORBIDDEN },
		{ "traversal com query",                RL("/../secret?x=1"),             STATUS_FORBIDDEN },
	};
	runErrCases("URI", cases, COUNT(cases));
}

struct UriCase {
	const char* name;
	std::string uri;
	std::string path;
	std::string query;
};

void testUriAccepted() {
	const UriCase cases[] = {
		{ "raiz",                              "/",                  "/",                 ""        },
		{ "arquivo",                           "/index.html",        "/index.html",       ""        },
		{ "diretorios aninhados",              "/images/icons/logo.png", "/images/icons/logo.png", "" },
		{ "barra final",                       "/dir/",              "/dir/",             ""        },
		{ "maiusculas preservadas",            "/Index.HTML",        "/Index.HTML",       ""        },
		{ "dois pontos no nome do arquivo",    "/files/report..txt", "/files/report..txt", ""       },
		{ "tres pontos nao e traversal",       "/...",               "/...",              ""        },
		{ "ponto no inicio do nome",           "/..a",               "/..a",              ""        },
		{ "ponto no fim do nome",              "/a../b",             "/a../b",            ""        },
		{ "arquivo oculto",                    "/.hidden",           "/.hidden",          ""        },
		{ "segmento '.' e permitido",          "/a/./b",             "/a/./b",            ""        },
		{ "barras duplas no meio",             "/a//b",              "/a//b",             ""        },
		{ "escape %41 vira A",                 "/%41",               "/A",                ""        },
		{ "escape minusculo %7e",              "/%7e",               "/~",                ""        },
		{ "escape maiusculo %7E",              "/%7E",               "/~",                ""        },
		{ "escape de espaco %20",              "/a%20b",             "/a b",              ""        },
		{ "escape no fim do path",             "/a%41",              "/aA",               ""        },
		{ "varios escapes seguidos",           "/%41%42%43",         "/ABC",              ""        },
		{ "sem dupla decodificacao %252e",     "/%252e%252e",        "/%2e%2e",           ""        },
		{ "%25 vira %",                        "/a%25b",             "/a%b",              ""        },
		{ "%3f no path nao inicia a query",    "/a%3fb",             "/a?b",              ""        },
		{ "%23 no path",                       "/a%23b",             "/a#b",              ""        },
		{ "query simples",                     "/search?q=cpp",      "/search",           "q=cpp"   },
		{ "query vazia",                       "/?",                 "/",                 ""        },
		{ "query com varios parametros",       "/a?x=1&y=2",         "/a",                "x=1&y=2" },
		{ "query com outro ?",                 "/a?b?c",             "/a",                "b?c"     },
		{ "query com escape valido nao decodifica", "/a?q=%41",      "/a",                "q=%41"   },
		{ "query com %00 (nao decodificado)",  "/a?q=%00",           "/a",                "q=%00"   },
		{ "query com '..' nao e traversal",    "/a?p=../x",          "/a",                "p=../x"  },
	};

	for (size_t i = 0; i < COUNT(cases); ++i) {
		std::string name = std::string("URI aceita: ") + cases[i].name;
		HttpRequest r;
		feedAll(r, "GET " + cases[i].uri + " HTTP/1.1\r\n");

		bool ok = r.getState() == HttpRequest::PARSING_HEADERS && r.getErrorCode() == 0 &&
		          r.getUri() == cases[i].uri &&
		          r.getPath() == cases[i].path &&
		          r.getQueryString() == cases[i].query;
		check(ok, ok ? name : name + " [" + describe(r) + " path='" + r.getPath() +
		                         "' query='" + r.getQueryString() + "']");
	}
}

void testQueryString() {
	HttpRequest request;
	std::string buffer = "GET /search?q=cpp HTTP/1.1\r\n";

	request.feed(buffer);

	check(request.getState() == HttpRequest::PARSING_HEADERS, "Query: estado correto");
	check(request.getUri() == "/search?q=cpp",                "Query: URI original");
	check(request.getPath() == "/search",                     "Query: path separado");
	check(request.getQueryString() == "q=cpp",                "Query: string separada");
}

struct VersionCase {
	const char* version;
	int         expectedCode;
};

void testHttpVersions() {
	const VersionCase cases[] = {
		{ "HTTP/1.1",   0 },

		// bem formada, mas nao suportada -> 505
		{ "HTTP/1.0",   STATUS_VERSION_NOT_SUPPORTED },
		{ "HTTP/1.2",   STATUS_VERSION_NOT_SUPPORTED },
		{ "HTTP/1.9",   STATUS_VERSION_NOT_SUPPORTED },
		{ "HTTP/0.9",   STATUS_VERSION_NOT_SUPPORTED },
		{ "HTTP/2.0",   STATUS_VERSION_NOT_SUPPORTED },
		{ "HTTP/3.0",   STATUS_VERSION_NOT_SUPPORTED },
		{ "HTTP/9.9",   STATUS_VERSION_NOT_SUPPORTED },

		// mal formada -> 400
		{ "HTTP/1",     STATUS_BAD_REQUEST },
		{ "HTTP/1.",    STATUS_BAD_REQUEST },
		{ "HTTP/.1",    STATUS_BAD_REQUEST },
		{ "HTTP/1.10",  STATUS_BAD_REQUEST },
		{ "HTTP/11.1",  STATUS_BAD_REQUEST },
		{ "HTTPS/1.1",  STATUS_BAD_REQUEST },
		{ "http/1.1",   STATUS_BAD_REQUEST },
		{ "XTTP/1.1",   STATUS_BAD_REQUEST },
		{ "HTTP-1.1",   STATUS_BAD_REQUEST },
		{ "HTTP/1,1",   STATUS_BAD_REQUEST },
		{ "HTTP/1.x",   STATUS_BAD_REQUEST },
		{ "HTTP/a.1",   STATUS_BAD_REQUEST },
		{ "HTTP/1.1\r", STATUS_BAD_REQUEST },
		{ "HTTP/ 1.1",  STATUS_BAD_REQUEST },
		{ "HTTP",       STATUS_BAD_REQUEST },
		{ "HTTP/",      STATUS_BAD_REQUEST },
		{ "1.1",        STATUS_BAD_REQUEST },
		{ "",           STATUS_BAD_REQUEST }
	};

	for (size_t i = 0; i < COUNT(cases); ++i) {
		HttpRequest request;
		std::string buffer = std::string("GET / ") + cases[i].version + "\r\n";

		request.feed(buffer);

		std::string name = std::string("Versao '") + cases[i].version + "'";

		if (cases[i].expectedCode == 0) {
			check(request.getState() == HttpRequest::PARSING_HEADERS &&
			      request.getErrorCode() == 0 &&
			      request.getHttpVersion() == cases[i].version,
			      name + ": aceita e guardada");
		} else {
			expectError(request, cases[i].expectedCode,
			            name + ": erro " + num(cases[i].expectedCode));
		}
	}
}

// ---------------------------------------------------------------------------
// Request line: enquadramento (linha incompleta, sobra, CRLF iniciais, ...)
// ---------------------------------------------------------------------------

void testIncompleteLine() {
	HttpRequest request;
	std::string buffer = "GET / HTTP/1.1";

	request.feed(buffer);

	check(request.getState() == HttpRequest::PARSING_REQUEST_LINE, "Linha incompleta: continua aguardando");
	check(request.getErrorCode() == 0,                             "Linha incompleta: sem erro");
	check(buffer == "GET / HTTP/1.1",                              "Linha incompleta: buffer preservado");
}

void testIncompleteLineCrOnly() {
	HttpRequest request;
	std::string buffer = "GET / HTTP/1.1\r";

	request.feed(buffer);

	check(request.getState() == HttpRequest::PARSING_REQUEST_LINE && request.getErrorCode() == 0,
	      "CR sem LF: continua aguardando");
	check(buffer == "GET / HTTP/1.1\r", "CR sem LF: buffer preservado");

	buffer += "\n";
	request.feed(buffer);
	check(request.getState() == HttpRequest::PARSING_HEADERS, "CR + LF chegando depois: linha aceita");
}

void testBareLfDoesNotTerminate() {
	HttpRequest request;
	std::string input = "GET / HTTP/1.1\nHost: localhost\n\n";
	std::string buffer = input;

	request.feed(buffer);

	check(request.getState() == HttpRequest::PARSING_REQUEST_LINE && request.getErrorCode() == 0,
	      "LF sem CR: nao encerra a linha (continua aguardando)");
	check(buffer == input, "LF sem CR: buffer preservado");
}

void testRequestLineInPieces() {
	HttpRequest request;
	std::string buffer = "GE";

	request.feed(buffer);
	check(request.getState() == HttpRequest::PARSING_REQUEST_LINE, "Linha em partes: 'GE' aguarda");

	buffer += "T /index.html HT";
	request.feed(buffer);
	check(request.getState() == HttpRequest::PARSING_REQUEST_LINE, "Linha em partes: sem CRLF aguarda");
	check(request.getErrorCode() == 0, "Linha em partes: sem erro enquanto incompleta");

	buffer += "TP/1.1\r\n";
	request.feed(buffer);
	check(request.getState() == HttpRequest::PARSING_HEADERS, "Linha em partes: completa ao receber o resto");
	check(request.getPath() == "/index.html", "Linha em partes: path correto");
}

void testAdditionalData() {
	HttpRequest request;
	std::string buffer = "GET / HTTP/1.1\r\nHost: localhost\r\n";

	request.feed(buffer);

	check(request.getState() == HttpRequest::PARSING_HEADERS, "Dados adicionais: primeira linha aceita");
	check(buffer == "Host: localhost\r\n",                     "Dados adicionais: restante preservado");
}

void testEmptyLinesBeforeRequest() {
	{
		HttpRequest request;
		std::string buffer = "\r\nGET / HTTP/1.1\r\n";
		request.feed(buffer);

		check(request.getState() == HttpRequest::PARSING_HEADERS, "Linha vazia inicial: requisicao aceita");
		check(request.getMethod() == "GET",                       "Linha vazia inicial: metodo correto");
	}
	{
		HttpRequest request;
		std::string buffer = "\r\n\r\n\r\nGET / HTTP/1.1\r\n";
		request.feed(buffer);

		check(request.getState() == HttpRequest::PARSING_HEADERS && request.getMethod() == "GET",
		      "Varias linhas vazias iniciais: requisicao aceita");
	}
	{
		HttpRequest request;
		std::string buffer = "\r\n";
		request.feed(buffer);

		check(request.getState() == HttpRequest::PARSING_REQUEST_LINE && request.getErrorCode() == 0,
		      "So uma linha vazia: continua aguardando");
		check(buffer.empty(), "So uma linha vazia: consumida");
	}
}

std::string requestLineOfLength(size_t total) {
	// "GET " + path + " HTTP/1.1"  => path tem total - 13 bytes
	return "GET /" + std::string(total - 4 - 1 - 9, 'a') + " HTTP/1.1";
}

void testRequestLineLengthBoundary() {
	{
		HttpRequest request;
		feedAll(request, requestLineOfLength(kMaxRequestLine) + "\r\n");
		check(request.getState() == HttpRequest::PARSING_HEADERS && request.getErrorCode() == 0,
		      "Linha no limite exato: aceita");
	}
	{
		HttpRequest request;
		feedAll(request, requestLineOfLength(kMaxRequestLine + 1) + "\r\n");
		expectError(request, STATUS_URI_TOO_LONG, "Linha 1 byte acima do limite: STATUS_URI_TOO_LONG");
	}
	{
		HttpRequest request;
		feedAll(request, requestLineOfLength(kMaxRequestLine));
		check(request.getState() == HttpRequest::PARSING_REQUEST_LINE && request.getErrorCode() == 0,
		      "Linha no limite sem CRLF: continua aguardando");
	}
	{
		HttpRequest request;
		feedAll(request, requestLineOfLength(kMaxRequestLine + 1));
		expectError(request, STATUS_URI_TOO_LONG,
		            "Linha acima do limite sem CRLF: erro imediato, sem esperar o fim da linha");
	}
	{
		HttpRequest request;
		feedAll(request, "GET /" + std::string(kHuge * 10, 'a') + " HTTP/1.1\r\n");
		expectError(request, STATUS_URI_TOO_LONG, "Linha enorme: STATUS_URI_TOO_LONG");
	}
}

// ===========================================================================
// Headers
// ===========================================================================

void testValidHeaders() {
	HttpRequest request;
	std::string buffer =
		"GET / HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"User-Agent: WebservTest/1.0\r\n"
		"Accept: */*\r\n"
		"\r\n";

	request.feed(buffer);

	check(request.getState() == HttpRequest::REQUEST_COMPLETE, "Headers validos: requisicao completa");
	check(request.getErrorCode() == 0,                         "Headers validos: sem erro");
	check(buffer.empty(),                                      "Headers validos: buffer consumido");
	check(request.getHeader("user-agent") == "WebservTest/1.0", "Headers validos: User-Agent");
	check(request.getHeader("accept") == "*/*",                 "Headers validos: Accept");
}

void testHeaderErrors() {
	const ErrCase cases[] = {
		// sem Host / Host invalido
		{ "sem headers (Host ausente)",          REQLINE "\r\n",                         STATUS_BAD_REQUEST },
		{ "Host ausente",                        REQLINE "User-Agent: x\r\n\r\n",        STATUS_BAD_REQUEST },
		{ "Host vazio",                          REQLINE "Host:\r\n\r\n",                STATUS_BAD_REQUEST },
		{ "Host so com espacos",                 REQLINE "Host:    \r\n\r\n",            STATUS_BAD_REQUEST },
		{ "Host com espaco no meio",             REQLINE "Host: a b\r\n\r\n",            STATUS_BAD_REQUEST },
		{ "Host com barra",                      REQLINE "Host: a/b\r\n\r\n",            STATUS_BAD_REQUEST },
		{ "Host com @",                          REQLINE "Host: user@evil.com\r\n\r\n",  STATUS_BAD_REQUEST },
		{ "Host com %",                          REQLINE "Host: a%20b\r\n\r\n",          STATUS_BAD_REQUEST },
		{ "Host com aspas",                      REQLINE "Host: ex\"ample\r\n\r\n",      STATUS_BAD_REQUEST },
		{ "Host com virgula",                    REQLINE "Host: a,b\r\n\r\n",            STATUS_BAD_REQUEST },
		{ "Host com byte alto",                  REQLINE "Host: ex\303\251.com\r\n\r\n", STATUS_BAD_REQUEST },

		// linhas de header malformadas
		{ "header sem dois-pontos",              GETH "InvalidHeader\r\n\r\n",           STATUS_BAD_REQUEST },
		{ "nome com espaco",                     GETH "Bad Header: v\r\n\r\n",           STATUS_BAD_REQUEST },
		{ "espaco antes dos dois-pontos",        GETH "X-A : v\r\n\r\n",                 STATUS_BAD_REQUEST },
		{ "TAB antes dos dois-pontos",           GETH "X-A\t: v\r\n\r\n",                STATUS_BAD_REQUEST },
		{ "nome vazio",                          GETH ": v\r\n\r\n",                     STATUS_BAD_REQUEST },
		{ "nome com parenteses",                 GETH "X(A): v\r\n\r\n",                 STATUS_BAD_REQUEST },
		{ "nome com byte alto",                  GETH "X-\303\251: v\r\n\r\n",           STATUS_BAD_REQUEST },
		{ "obs-fold (continuacao com espaco)",   GETH "X-A: b\r\n c: d\r\n\r\n",         STATUS_BAD_REQUEST },
		{ "linha iniciada com TAB",              GETH "\tX-A: v\r\n\r\n",                STATUS_BAD_REQUEST },

		// valores com caracteres proibidos
		{ "valor com controle \\001",            GETH "X-A: a\001b\r\n\r\n",             STATUS_BAD_REQUEST },
		{ "valor com DEL",                       GETH "X-A: a\177b\r\n\r\n",             STATUS_BAD_REQUEST },
		{ "valor com byte alto (UTF-8)",         GETH "X-A: a\303\251b\r\n\r\n",         STATUS_BAD_REQUEST },
		{ "valor com CR solto no meio",          GETH "X-A: a\rb\r\n\r\n",               STATUS_BAD_REQUEST },
		{ "valor com LF solto no meio",          GETH "X-A: a\nb\r\n\r\n",               STATUS_BAD_REQUEST },
		{ "valor com CR solto no fim",           GETH "X-A: abc\r\r\n\r\n",              STATUS_BAD_REQUEST },
		{ "valor com vertical tab",              GETH "X-A: a\013b\r\n\r\n",             STATUS_BAD_REQUEST },
		{ "valor com form feed",                 GETH "X-A: a\014b\r\n\r\n",             STATUS_BAD_REQUEST },

		// duplicatas proibidas
		{ "Host duplicado",                      GETH "Host: example.com\r\n\r\n",       STATUS_BAD_REQUEST },
		{ "Host duplicado (caixa diferente)",    GETH "hOsT: example.com\r\n\r\n",       STATUS_BAD_REQUEST },
		{ "Host duplicado igual",                GETH "Host: localhost\r\n\r\n",         STATUS_BAD_REQUEST },
		{ "Content-Length duplicado",            GETH "Content-Length: 0\r\nContent-Length: 0\r\n\r\n", STATUS_BAD_REQUEST },
		{ "Content-Length duplicado (caixa)",    GETH "content-length: 0\r\nCONTENT-LENGTH: 0\r\n\r\n", STATUS_BAD_REQUEST },
		{ "Content-Length duplicado diferente",  GETH "Content-Length: 5\r\nContent-Length: 6\r\n\r\n", STATUS_BAD_REQUEST },
		{ "Transfer-Encoding duplicado",         GETH "Transfer-Encoding: chunked\r\nTransfer-Encoding: chunked\r\n\r\n", STATUS_BAD_REQUEST },
		{ "Content-Type duplicado",              GETH "Content-Type: a\r\nContent-Type: b\r\n\r\n", STATUS_BAD_REQUEST },

		// conflito de enquadramento (request smuggling)
		{ "CL antes de TE",                      GETH "Content-Length: 5\r\nTransfer-Encoding: chunked\r\n\r\n", STATUS_BAD_REQUEST },
		{ "TE antes de CL",                      GETH "Transfer-Encoding: chunked\r\nContent-Length: 5\r\n\r\n", STATUS_BAD_REQUEST },
		{ "TE com CL zero",                      GETH "Transfer-Encoding: chunked\r\nContent-Length: 0\r\n\r\n", STATUS_BAD_REQUEST },
	};
	runErrCases("Headers", cases, COUNT(cases));
}

struct HeaderCase {
	const char* name;
	std::string lines;   // linhas extras (cada uma com seu CRLF), apos o Host
	const char* key;
	std::string value;
};

void testHeadersAccepted() {
	const HeaderCase cases[] = {
		{ "valor vazio",                     "X-Empty:\r\n",                 "x-empty",  "" },
		{ "valor so com espacos",            "X-Blank:   \r\n",              "x-blank",  "" },
		{ "espacos das pontas removidos",    "X-A:   v  \r\n",               "x-a",      "v" },
		{ "TABs das pontas removidos",       "X-A:\tv\t\r\n",                "x-a",      "v" },
		{ "espacos internos preservados",    "X-A: a  b\r\n",                "x-a",      "a  b" },
		{ "TAB interno preservado",          "X-A: a\tb\r\n",                "x-a",      "a\tb" },
		{ "sem espaco depois dos ':'",       "X-A:v\r\n",                    "x-a",      "v" },
		{ "':' dentro do valor",             "X-U: http://a:80/b\r\n",       "x-u",      "http://a:80/b" },
		{ "nome em minusculas",              "X-MiXeD-NaMe: v\r\n",          "x-mixed-name", "v" },
		{ "caixa do valor preservada",       "X-A: AbC dEf\r\n",             "x-a",      "AbC dEf" },
		{ "simbolos tchar no nome",          "X-Weird_Name.1~!#$%&'*+^`|: v\r\n", "x-weird_name.1~!#$%&'*+^`|", "v" },
		{ "valor com virgula preservado",    "Accept: a, b\r\n",             "accept",   "a, b" },
		{ "duplicata junta com virgula",     "Accept: a\r\nAccept: b\r\n",   "accept",   "a,b" },
		{ "tres duplicatas",                 "Accept: a\r\nAccept: b\r\nAccept: c\r\n", "accept", "a,b,c" },
		{ "duplicata com caixa diferente",   "Accept: a\r\nACCEPT: b\r\n",   "accept",   "a,b" },
		{ "duplicata: segundo vazio ignorado", "Accept: a\r\nAccept:\r\n",   "accept",   "a" },
		{ "duplicata: primeiro vazio",       "Accept:\r\nAccept: b\r\n",     "accept",   "b" },
	};

	for (size_t i = 0; i < COUNT(cases); ++i) {
		std::string name = std::string("Header aceito: ") + cases[i].name;
		HttpRequest r;
		feedAll(r, GETH + cases[i].lines + "\r\n");

		bool ok = r.getState() == HttpRequest::REQUEST_COMPLETE && r.getErrorCode() == 0 &&
		          r.hasHeader(cases[i].key) && r.getHeader(cases[i].key) == cases[i].value;
		check(ok, ok ? name : name + " [" + describe(r) + " valor='" +
		                         r.getHeader(cases[i].key) + "']");
	}
}

void testHostAccepted() {
	const char* hosts[] = {
		"localhost", "localhost:8080", "127.0.0.1", "127.0.0.1:80", "[::1]", "[::1]:8080",
		"example.com", "EXAMPLE.COM", "sub.example.com:443", "a_b.c-d"
	};

	for (size_t i = 0; i < COUNT(hosts); ++i) {
		HttpRequest r;
		feedAll(r, std::string(REQLINE "Host: ") + hosts[i] + "\r\n\r\n");
		expectComplete(r, std::string("Host aceito: ") + hosts[i]);
	}
}

void testHeaderNameIsCaseInsensitive() {
	HttpRequest request;
	std::string buffer = REQLINE "hOsT: localhost\r\n\r\n";

	request.feed(buffer);

	expectComplete(request, "Header case-insensitive: requisicao completa");
	check(request.hasHeader("host") && request.getHeader("host") == "localhost",
	      "Header case-insensitive: guardado em minusculas");
	check(!request.hasHeader("Host") && !request.hasHeader("hOsT"),
	      "Header case-insensitive: chaves nao minusculas nao existem");
}

struct ConnCase {
	const char* name;
	std::string lines;
	bool        keepAlive;
};

void testConnectionHeader() {
	const ConnCase cases[] = {
		{ "ausente",                          "",                                         true  },
		{ "close",                            "Connection: close\r\n",                    false },
		{ "Close (maiusculo inicial)",        "Connection: Close\r\n",                    false },
		{ "CLOSE",                            "Connection: CLOSE\r\n",                    false },
		{ "keep-alive",                       "Connection: keep-alive\r\n",               true  },
		{ "closed nao e close",               "Connection: closed\r\n",                   true  },
		{ "keep-alive, close",                "Connection: keep-alive, close\r\n",        false },
		{ "close, keep-alive",                "Connection: close, keep-alive\r\n",        false },
		{ "virgula antes de close",           "Connection: ,close\r\n",                   false },
		{ "espacos em volta de close",        "Connection: upgrade ,  close \r\n",        false },
		{ "valor vazio",                      "Connection:\r\n",                          true  },
		{ "duas linhas (keep-alive e close)", "Connection: keep-alive\r\nConnection: close\r\n", false },
		{ "duas linhas (close e keep-alive)", "Connection: close\r\nConnection: keep-alive\r\n", false },
	};

	for (size_t i = 0; i < COUNT(cases); ++i) {
		std::string name = std::string("Connection ") + cases[i].name;
		HttpRequest r;
		feedAll(r, GETH + cases[i].lines + "\r\n");

		check(r.getState() == HttpRequest::REQUEST_COMPLETE && r.getKeepAlive() == cases[i].keepAlive,
		      name + (cases[i].keepAlive ? ": keep-alive" : ": close"));
	}
}

void testHeadersInPieces() {
	HttpRequest request;
	std::string buffer = REQLINE;

	request.feed(buffer);
	check(request.getState() == HttpRequest::PARSING_HEADERS, "Headers em partes: request line aceita");

	buffer += "Host: local";
	request.feed(buffer);
	check(request.getState() == HttpRequest::PARSING_HEADERS && request.getErrorCode() == 0,
	      "Headers em partes: header incompleto aguarda");

	buffer += "host\r\nX-A: b\r\n";
	request.feed(buffer);
	check(request.getState() == HttpRequest::PARSING_HEADERS,
	      "Headers em partes: sem linha em branco ainda aguarda");

	buffer += "\r";
	request.feed(buffer);
	check(request.getState() == HttpRequest::PARSING_HEADERS && request.getErrorCode() == 0,
	      "Headers em partes: CR final sozinho aguarda");

	buffer += "\n";
	request.feed(buffer);
	expectComplete(request, "Headers em partes: completa ao receber o LF final");
	check(request.getHeader("host") == "localhost", "Headers em partes: Host reconstruido");
	check(request.getHeader("x-a") == "b",          "Headers em partes: X-A");
}

void testHeadersByteByByte() {
	HttpRequest request;
	std::string data =
		"GET /a/b?x=1 HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"X-One: 1\r\n"
		"X-Two: 2\r\n"
		"\r\n";

	check(feedByteByByteStrict(request, data), "Headers byte a byte: nao termina antes do ultimo byte");
	expectComplete(request, "Headers byte a byte: requisicao completa");
	check(request.getPath() == "/a/b" && request.getQueryString() == "x=1",
	      "Headers byte a byte: path e query");
	check(request.getHeader("x-two") == "2", "Headers byte a byte: ultimo header");
}

void testHeadersWithoutTerminatorWaits() {
	HttpRequest request;
	std::string buffer = GETH "X-A: b\r\n";

	request.feed(buffer);

	check(request.getState() == HttpRequest::PARSING_HEADERS && request.getErrorCode() == 0,
	      "Headers sem linha em branco: continua aguardando");
	check(buffer == "Host: localhost\r\nX-A: b\r\n",
	      "Headers sem linha em branco: buffer preservado");
}

void testHeaderCountBoundary() {
	for (int extra = 98; extra <= 100; ++extra) {
		// Host + extra headers. 99 extras = 100 headers (limite); 100 extras = 101.
		HttpRequest request;
		std::string buffer = GETH;
		for (int i = 0; i < extra; ++i)
			buffer += "X-Test-" + num(i) + ": value\r\n";
		buffer += "\r\n";

		request.feed(buffer);

		size_t total = static_cast<size_t>(extra) + 1;
		std::string name = num(total) + " headers";
		if (total <= kMaxHeaderCount)
			expectComplete(request, name + ": aceitos");
		else
			expectError(request, STATUS_HEADERS_TOO_LARGE, name + ": STATUS_HEADERS_TOO_LARGE");
	}
}

std::string headersOfBlockSize(size_t block) {
	// bloco = tudo entre a request line e a linha em branco final
	const std::string base = "Host: localhost\r\nX-Fill: ";
	return REQLINE + base + std::string(block - base.size(), 'a') + "\r\n\r\n";
}

void testHeadersSizeBoundary() {
	{
		HttpRequest request;
		feedAll(request, headersOfBlockSize(kMaxHeadersSize - 1));
		expectComplete(request, "Bloco de headers 1 byte abaixo do limite: aceito");
	}
	{
		HttpRequest request;
		feedAll(request, headersOfBlockSize(kMaxHeadersSize));
		expectError(request, STATUS_HEADERS_TOO_LARGE, "Bloco de headers no limite: STATUS_HEADERS_TOO_LARGE");
	}
	{
		HttpRequest request;
		feedAll(request, headersOfBlockSize(kMaxHeadersSize + 100));
		expectError(request, STATUS_HEADERS_TOO_LARGE, "Bloco de headers acima do limite: STATUS_HEADERS_TOO_LARGE");
	}
	{
		HttpRequest request;
		feedAll(request, std::string(REQLINE) + std::string(kMaxHeadersSize - 1, 'a'));
		check(request.getState() == HttpRequest::PARSING_HEADERS && request.getErrorCode() == 0,
		      "Headers sem terminador abaixo do limite: continua aguardando");
	}
	{
		HttpRequest request;
		feedAll(request, std::string(REQLINE) + std::string(kMaxHeadersSize, 'a'));
		expectError(request, STATUS_HEADERS_TOO_LARGE,
		            "Headers sem terminador no limite: erro imediato");
	}
	{
		HttpRequest request;
		feedAll(request, std::string(GETH "X-Fill: ") + std::string(kHuge, 'a') + "\r\n\r\n");
		expectError(request, STATUS_HEADERS_TOO_LARGE, "Header gigante: STATUS_HEADERS_TOO_LARGE");
	}
}

// ===========================================================================
// Content-Length e Transfer-Encoding (decisao de enquadramento do body)
// ===========================================================================

void testContentLengthErrors() {
	const ErrCase cases[] = {
		{ "abc",                    GETH "Content-Length: abc\r\n\r\n",     STATUS_BAD_REQUEST },
		{ "negativo",               GETH "Content-Length: -1\r\n\r\n",      STATUS_BAD_REQUEST },
		{ "com sinal +",            GETH "Content-Length: +5\r\n\r\n",      STATUS_BAD_REQUEST },
		{ "vazio",                  GETH "Content-Length:\r\n\r\n",         STATUS_BAD_REQUEST },
		{ "dois numeros",           GETH "Content-Length: 5 5\r\n\r\n",     STATUS_BAD_REQUEST },
		{ "lista 5, 5",             GETH "Content-Length: 5, 5\r\n\r\n",    STATUS_BAD_REQUEST },
		{ "hexa 0x10",              GETH "Content-Length: 0x10\r\n\r\n",    STATUS_BAD_REQUEST },
		{ "hexa sem prefixo (1a)",  GETH "Content-Length: 1a\r\n\r\n",      STATUS_BAD_REQUEST },
		{ "notacao cientifica",     GETH "Content-Length: 1e3\r\n\r\n",     STATUS_BAD_REQUEST },
		{ "decimal 5.0",            GETH "Content-Length: 5.0\r\n\r\n",     STATUS_BAD_REQUEST },
		{ "com unidade",            GETH "Content-Length: 5b\r\n\r\n",      STATUS_BAD_REQUEST },
		{ "um byte acima do limite", GETH "Content-Length: 1048577\r\n\r\n", STATUS_CONTENT_TOO_LARGE },
		{ "muito acima do limite",  GETH "Content-Length: 99999999\r\n\r\n", STATUS_CONTENT_TOO_LARGE },
	};
	runErrCases("Content-Length", cases, COUNT(cases));
}

void testContentLengthOverflow() {
	HttpRequest request;
	std::string buffer = GETH "Content-Length: " + std::string(100, '9') + "\r\n\r\n";

	request.feed(buffer);

	check(request.getState() == HttpRequest::REQUEST_ERROR &&
	      (request.getErrorCode() == STATUS_BAD_REQUEST ||
	       request.getErrorCode() == STATUS_CONTENT_TOO_LARGE),
	      "Content-Length enorme: erro apropriado");
}

void testContentLengthSizeMax() {
	// 2^64 - 1 (e 2^64, que estoura size_t): nunca pode ser aceito.
	const char* values[] = { "18446744073709551615", "18446744073709551616", "99999999999999999999" };
	for (size_t i = 0; i < COUNT(values); ++i) {
		HttpRequest request;
		feedAll(request, std::string(GETH "Content-Length: ") + values[i] + "\r\n\r\n");
		check(request.getState() == HttpRequest::REQUEST_ERROR &&
		      (request.getErrorCode() == STATUS_BAD_REQUEST ||
		       request.getErrorCode() == STATUS_CONTENT_TOO_LARGE),
		      std::string("Content-Length ") + values[i] + ": rejeitado");
	}
}

void testContentLengthHeaderOnly() {
	{
		HttpRequest request;
		feedAll(request, GETH "Content-Length: 0\r\n\r\n");
		expectComplete(request, "Content-Length zero: requisicao completa");
	}
	{
		HttpRequest request;
		feedAll(request, GETH "Content-Length: 00\r\n\r\n");
		expectComplete(request, "Content-Length 00: tratado como zero");
	}
	{
		HttpRequest request;
		std::string buffer = lengthHeaders(10);
		request.feed(buffer);

		check(request.getState() == HttpRequest::PARSING_BODY, "Content-Length positivo: estado PARSING_BODY");
		check(request.getErrorCode() == 0,                     "Content-Length positivo: sem erro");
		check(buffer.empty(),                                  "Content-Length positivo: headers consumidos");
	}
}

struct TeCase {
	const char* name;
	const char* value;
	int         code;   // 0 = aceito (chunked), senao o erro esperado
};

void testTransferEncodingValues() {
	const TeCase cases[] = {
		{ "chunked",                     "chunked",                    0 },
		{ "Chunked",                     "Chunked",                    0 },
		{ "CHUNKED",                     "CHUNKED",                    0 },
		{ "espacos nas pontas",          " chunked ",                  0 },
		{ "TABs nas pontas",             "\tchunked\t",                0 },

		{ "gzip",                        "gzip",                       STATUS_NOT_IMPLEMENTED },
		{ "identity",                    "identity",                   STATUS_NOT_IMPLEMENTED },
		{ "gzip, chunked",               "gzip, chunked",              STATUS_NOT_IMPLEMENTED },
		{ "gzip,br",                     "gzip,br",                    STATUS_NOT_IMPLEMENTED },
		{ "gzip, br",                    "gzip, br",                   STATUS_NOT_IMPLEMENTED },
		{ "lista longa terminada em chunked", "gzip,br,deflate,chunked", STATUS_NOT_IMPLEMENTED },

		{ "chunked, gzip (chunked nao e o ultimo)", "chunked, gzip",   STATUS_BAD_REQUEST },
		{ "chunked, chunked",            "chunked, CHUNKED",           STATUS_BAD_REQUEST },
		{ "chunked repetido, caixas",    "Chunked,CHUNKED",            STATUS_BAD_REQUEST },
		{ "vazio",                       "",                           STATUS_BAD_REQUEST },
		{ "virgula inicial",             ",chunked",                   STATUS_BAD_REQUEST },
		{ "virgula final",               "chunked,",                   STATUS_BAD_REQUEST },
		{ "tokens vazios consecutivos",  "chunked,,gzip",              STATUS_BAD_REQUEST },
		{ "somente virgulas",            ",,",                         STATUS_BAD_REQUEST },
		{ "espaco dentro do token",      "chunk ed",                   STATUS_BAD_REQUEST },
		{ "TAB dentro do token",         "chunk\ted",                  STATUS_BAD_REQUEST },
		{ "caractere invalido",          "gzip@",                      STATUS_BAD_REQUEST },
		{ "chunked com parametro",       "chunked;q=1",                STATUS_BAD_REQUEST },
		{ "aspas",                       "\"chunked\"",                STATUS_BAD_REQUEST },
	};

	for (size_t i = 0; i < COUNT(cases); ++i) {
		std::string name = std::string("Transfer-Encoding '") + cases[i].name + "'";
		HttpRequest r;
		feedAll(r, std::string("POST /upload HTTP/1.1\r\n" HOST "Transfer-Encoding: ") +
		           cases[i].value + "\r\n\r\n");

		if (cases[i].code == 0)
			expectState(r, HttpRequest::PARSING_BODY, name + ": aceito (chunked)");
		else
			expectError(r, cases[i].code, name + ": erro " + num(cases[i].code));
	}
}

void testTransferEncodingOnGet() {
	HttpRequest request;
	feedAll(request, GETH "Transfer-Encoding: chunked\r\n\r\n");
	expectState(request, HttpRequest::PARSING_BODY, "GET com chunked: espera o body");
}

// ===========================================================================
// Body com Content-Length
// ===========================================================================

void testNoBodyKeepsExtraBytes() {
	// Sem Content-Length nem Transfer-Encoding o body e vazio (RFC 9112, 6.3):
	// o que vier depois dos headers e outra requisicao.
	HttpRequest request;
	std::string next = "GET /next HTTP/1.1\r\n";
	std::string buffer = GETH "\r\n" + next;

	request.feed(buffer);

	expectComplete(request, "Sem body: requisicao completa nos headers");
	check(request.getBody().empty(), "Sem body: body vazio");
	check(buffer == next,            "Sem body: bytes seguintes nao sao consumidos como body");
}

void testBodyContentLengthComplete() {
	HttpRequest request;
	std::string buffer = lengthHeaders(5) + "hello";

	request.feed(buffer);

	expectComplete(request, "Body CL: requisicao completa");
	check(request.getBody() == "hello", "Body CL: conteudo do body");
	check(buffer.empty(),               "Body CL: buffer consumido");
}

void testBodyContentLengthLeadingZeros() {
	HttpRequest request;
	std::string buffer = GETH "Content-Length: 05\r\n\r\nhello";

	request.feed(buffer);

	expectComplete(request, "Body CL com zero a esquerda: completo");
	check(request.getBody() == "hello", "Body CL com zero a esquerda: 5 bytes");
}

void testBodyContentLengthHeadersThenBody() {
	HttpRequest request;
	std::string buffer = lengthHeaders(5);

	request.feed(buffer);
	check(request.getState() == HttpRequest::PARSING_BODY, "Body CL separado: apos os headers espera o body");

	buffer += "hello";
	request.feed(buffer);

	expectComplete(request, "Body CL separado: completa quando o body chega");
	check(request.getBody() == "hello", "Body CL separado: conteudo do body");
}

void testBodyContentLengthInPieces() {
	HttpRequest request;
	std::string buffer = lengthHeaders(10) + "hel";

	request.feed(buffer);
	check(request.getState() == HttpRequest::PARSING_BODY, "Body CL em partes: body parcial continua aguardando");
	check(request.getErrorCode() == 0,                     "Body CL em partes: body parcial sem erro");

	buffer += "lo wo";
	request.feed(buffer);
	check(request.getState() == HttpRequest::PARSING_BODY, "Body CL em partes: ainda faltam bytes");

	buffer += "rld";   // so 2 bytes sao necessarios; 1 byte a mais fica no buffer
	request.feed(buffer);

	expectComplete(request, "Body CL em partes: completa ao receber todos os bytes");
	check(request.getBody() == "hello worl", "Body CL em partes: body tem exatamente Content-Length bytes");
	check(buffer == "d",                     "Body CL em partes: byte excedente fica no buffer");
}

void testBodyContentLengthByteByByte() {
	HttpRequest request;
	std::string data = lengthHeaders(11) + "hello world";

	check(feedByteByByteStrict(request, data),
	      "Body CL byte a byte: nao completa nem erra antes do ultimo byte");
	expectComplete(request, "Body CL byte a byte: requisicao completa");
	check(request.getBody() == "hello world", "Body CL byte a byte: conteudo do body");
}

void testBodyContentLengthExtraBytes() {
	HttpRequest request;
	std::string next = "GET /next HTTP/1.1\r\n";
	std::string buffer = lengthHeaders(5) + "hello" + next;

	request.feed(buffer);

	expectComplete(request, "Body CL + proxima requisicao: completa");
	check(request.getBody() == "hello", "Body CL + proxima requisicao: body com exatamente 5 bytes");
	check(buffer == next,               "Body CL + proxima requisicao: bytes seguintes preservados no buffer");
}

void testBodyContentLengthBinary() {
	// O body pode ter CRLF, CRLFCRLF e byte nulo: nada disso e delimitador.
	std::string body = "ab\r\n\r\ncd";
	body += '\0';
	body += "ef";

	HttpRequest request;
	std::string buffer = lengthHeaders(body.size()) + body;

	request.feed(buffer);

	expectComplete(request, "Body CL binario: requisicao completa");
	check(request.getBody() == body, "Body CL binario: CRLF e byte nulo preservados");
}

void testBodyContentLengthAllByteValues() {
	std::string body;
	for (int i = 0; i < 256; ++i)
		body += static_cast<char>(i);

	HttpRequest request;
	std::string buffer = lengthHeaders(body.size()) + body;
	request.feed(buffer);

	expectComplete(request, "Body CL com os 256 valores de byte: completo");
	check(request.getBody() == body, "Body CL com os 256 valores de byte: preservados");
}

void testBodyContentLengthMissingBytes() {
	HttpRequest request;
	std::string buffer = lengthHeaders(10) + "abcd";

	request.feed(buffer);

	check(request.getState() == HttpRequest::PARSING_BODY, "Body CL incompleto: continua em PARSING_BODY");
	check(request.getErrorCode() == 0,                     "Body CL incompleto: sem erro");
	check(buffer == "abcd",                                "Body CL incompleto: bytes ficam no buffer");
}

void testZeroContentLengthKeepsExtraBytes() {
	HttpRequest request;
	std::string next = "GET /next HTTP/1.1\r\n";
	std::string buffer = lengthHeaders(0) + next;

	request.feed(buffer);

	expectComplete(request, "Content-Length 0 + proxima requisicao: completa");
	check(request.getBody().empty(), "Content-Length 0 + proxima requisicao: body vazio");
	check(buffer == next,            "Content-Length 0 + proxima requisicao: bytes seguintes preservados");
}

void testBodyContentLengthLarge() {
	std::string body;
	body.reserve(kDefaultMaxBody);
	for (size_t i = 0; i < kDefaultMaxBody; ++i)
		body += static_cast<char>(i % 251);

	HttpRequest request;
	std::string buffer = lengthHeaders(body.size());
	request.feed(buffer);
	check(request.getState() == HttpRequest::PARSING_BODY, "Body CL de 1 MB: espera o body");

	for (size_t pos = 0; pos < body.size(); pos += 4096) {
		buffer.append(body, pos, 4096);
		request.feed(buffer);
	}

	expectComplete(request, "Body CL de 1 MB em pedacos de 4 KB: completo");
	check(request.getBody() == body, "Body CL de 1 MB: conteudo identico");
	check(buffer.empty(),            "Body CL de 1 MB: buffer consumido");
}

// ===========================================================================
// Limite de body (max_body vindo do ServerConfig via construtor)
// ===========================================================================

void testDefaultLimitBoundary() {
	HttpRequest request;
	feedAll(request, lengthHeaders(kDefaultMaxBody));
	expectState(request, HttpRequest::PARSING_BODY, "Limite padrao: Content-Length igual ao limite (1 MB) aceito");
}

void testCustomLimits() {
	{
		HttpRequest request(10);
		feedAll(request, lengthHeaders(10));
		expectState(request, HttpRequest::PARSING_BODY, "Limite 10: Content-Length 10 aceito");
	}
	{
		HttpRequest request(10);
		feedAll(request, lengthHeaders(11));
		expectError(request, STATUS_CONTENT_TOO_LARGE, "Limite 10: Content-Length 11 STATUS_CONTENT_TOO_LARGE");
	}
	{
		// Pega o erro de continuar comparando com o limite padrao em vez
		// do limite recebido no construtor.
		HttpRequest request(10 * 1024 * 1024);
		feedAll(request, lengthHeaders(5 * 1024 * 1024));
		expectState(request, HttpRequest::PARSING_BODY, "Limite 10 MB: Content-Length 5 MB aceito");
	}
	{
		HttpRequest request(10);
		feedAll(request, GETH "Content-Length: " + std::string(100, '9') + "\r\n\r\n");
		expectError(request, STATUS_BAD_REQUEST, "Limite 10: Content-Length com overflow STATUS_BAD_REQUEST");
	}
	{
		HttpRequest request(0);
		feedAll(request, lengthHeaders(0));
		expectComplete(request, "Limite 0: Content-Length 0 aceito");
	}
	{
		HttpRequest request(0);
		feedAll(request, lengthHeaders(1));
		expectError(request, STATUS_CONTENT_TOO_LARGE, "Limite 0: Content-Length 1 STATUS_CONTENT_TOO_LARGE");
	}
	{
		HttpRequest request(0);
		feedAll(request, chunkedHeaders() + "0\r\n\r\n");
		expectComplete(request, "Limite 0: chunked vazio aceito");
	}
	{
		HttpRequest request(0);
		feedAll(request, chunkedHeaders() + "1\r\nX\r\n0\r\n\r\n");
		expectError(request, STATUS_CONTENT_TOO_LARGE, "Limite 0: chunked de 1 byte STATUS_CONTENT_TOO_LARGE");
	}
}

// ===========================================================================
// Body chunked
// ===========================================================================

struct BodyCase {
	const char* name;
	std::string payload;   // tudo depois dos headers
	std::string body;      // body esperado
};

void testChunkedAccepted() {
	const BodyCase cases[] = {
		{ "so o chunk final",              "0\r\n\r\n",                                    "" },
		{ "chunk final com extensao",      "0;ext=1\r\n\r\n",                              "" },
		{ "chunk final com zeros",         "00\r\n\r\n",                                   "" },
		{ "chunk final com varias extensoes", "0;a;b=c\r\n\r\n",                          "" },
		{ "um chunk",                      "5\r\nhello\r\n0\r\n\r\n",                      "hello" },
		{ "varios chunks",                 "5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n",       "hello world" },
		{ "tamanho hexa minusculo",        "a\r\n0123456789\r\n0\r\n\r\n",                 "0123456789" },
		{ "tamanho hexa maiusculo",        "A\r\n0123456789\r\n0\r\n\r\n",                 "0123456789" },
		{ "tamanho de dois digitos (1F)",  std::string("1F\r\n") + std::string(31, 'x') + "\r\n0\r\n\r\n", std::string(31, 'x') },
		{ "zeros a esquerda",              "005\r\nhello\r\n0\r\n\r\n",                    "hello" },
		{ "muitos zeros a esquerda",       "0000000000000005\r\nhello\r\n0\r\n\r\n",       "hello" },
		{ "extensao ignorada",             "5;name=value\r\nhello\r\n0\r\n\r\n",           "hello" },
		{ "extensao sem valor",            "5;name\r\nhello\r\n0\r\n\r\n",                 "hello" },
		{ "extensao entre aspas",          "5;a=\"x;y\"\r\nhello\r\n0\r\n\r\n",            "hello" },
		{ "espaco antes do ';'",           "5 ;a=b\r\nhello\r\n0\r\n\r\n",                 "hello" },
		{ "TAB antes do ';'",              "5\t;a\r\nhello\r\n0\r\n\r\n",                  "hello" },
		{ "extensao em varios chunks",     "5;a\r\nhello\r\n5;b=c\r\nworld\r\n0\r\n\r\n",  "helloworld" },
		{ "dados com CRLF",                "8\r\nab\r\n\r\ncd\r\n0\r\n\r\n",               "ab\r\n\r\ncd" },
		{ "dados que parecem o fim",       "5\r\n" "0\r\n\r\n" "\r\n" "0\r\n\r\n",         "0\r\n\r\n" },
		{ "chunk so com CRLF",             "2\r\n\r\n\r\n0\r\n\r\n",                       "\r\n" },
		{ "trailer",                       "5\r\nhello\r\n0\r\nX-Trailer: value\r\n\r\n",  "hello" },
		{ "varios trailers",               "0\r\nA: 1\r\nB: 2\r\n\r\n",                    "" },
		{ "trailer com valor vazio",       "0\r\nX:\r\n\r\n",                              "" },
		{ "trailer com espacos",           "0\r\nX:  v  \r\n\r\n",                         "" },
		{ "trailer com nome em maiusculas", "0\r\nX-CHECKSUM: abc\r\n\r\n",               "" },
		{ "chunk final com extensao e trailer", "3\r\nabc\r\n0;x\r\nT: v\r\n\r\n",        "abc" },
	};

	for (size_t i = 0; i < COUNT(cases); ++i) {
		std::string name = std::string("Chunked: ") + cases[i].name;
		std::string input = chunkedHeaders() + cases[i].payload;

		HttpRequest request;
		std::string buffer = input;
		request.feed(buffer);

		bool ok = request.getState() == HttpRequest::REQUEST_COMPLETE &&
		          request.getErrorCode() == 0 &&
		          request.getBody() == cases[i].body && buffer.empty();
		check(ok, ok ? name : name + " [" + describe(request) + " body='" + request.getBody() +
		                         "' sobra=" + num(buffer.size()) + "]");

		checkSplitEquivalence(name, input);
	}
}

void testChunkedHeadersThenBody() {
	HttpRequest request;
	std::string buffer = chunkedHeaders();

	request.feed(buffer);
	check(request.getState() == HttpRequest::PARSING_BODY, "Chunked separado: apos os headers espera o body");

	buffer += "5\r\nhello\r\n0\r\n\r\n";
	request.feed(buffer);

	expectComplete(request, "Chunked separado: completa quando os chunks chegam");
	check(request.getBody() == "hello", "Chunked separado: conteudo do body");
}

void testChunkedPartialStates() {
	{
		HttpRequest request;
		std::string buffer = chunkedHeaders() + "5\r\nhel";
		request.feed(buffer);

		check(request.getState() == HttpRequest::PARSING_BODY && request.getErrorCode() == 0,
		      "Chunked dados parciais: continua aguardando");
		check(buffer == "5\r\nhel", "Chunked dados parciais: buffer preservado");

		buffer += "lo\r\n0\r\n\r\n";
		request.feed(buffer);
		expectComplete(request, "Chunked dados parciais: completa ao receber o resto");
		check(request.getBody() == "hello", "Chunked dados parciais: conteudo do body");
	}
	{
		HttpRequest request;
		std::string buffer = chunkedHeaders() + "5";
		request.feed(buffer);

		check(request.getState() == HttpRequest::PARSING_BODY && request.getErrorCode() == 0,
		      "Chunked linha de tamanho cortada: continua aguardando");

		buffer += "\r\nhello\r\n0\r\n\r\n";
		request.feed(buffer);
		expectComplete(request, "Chunked linha de tamanho cortada: completa ao receber o resto");
		check(request.getBody() == "hello", "Chunked linha de tamanho cortada: conteudo do body");
	}
	{
		HttpRequest request;
		std::string buffer = chunkedHeaders() + "5\r";
		request.feed(buffer);
		check(request.getState() == HttpRequest::PARSING_BODY && request.getErrorCode() == 0,
		      "Chunked CR da linha de tamanho sem LF: aguarda");
	}
	{
		HttpRequest request;
		std::string buffer = chunkedHeaders() + "5\r\nhello";
		request.feed(buffer);
		check(request.getState() == HttpRequest::PARSING_BODY && request.getErrorCode() == 0,
		      "Chunked dados completos sem CRLF: aguarda");
		check(request.getBody().empty() && buffer == "5\r\nhello",
		      "Chunked dados completos sem CRLF: nada consumido ainda");
	}
	{
		HttpRequest request;
		std::string buffer = chunkedHeaders() + "5\r\nhello\r";
		request.feed(buffer);
		check(request.getState() == HttpRequest::PARSING_BODY && request.getErrorCode() == 0,
		      "Chunked CRLF apos os dados incompleto: aguarda");
	}
	{
		HttpRequest request;
		std::string buffer = chunkedHeaders() + "5\r\nhello\r\n";
		request.feed(buffer);
		check(request.getState() == HttpRequest::PARSING_BODY && request.getErrorCode() == 0,
		      "Chunked sem chunk final: continua em PARSING_BODY");
		check(request.getBody() == "hello" && buffer.empty(),
		      "Chunked sem chunk final: chunk completo ja consumido");
	}
	{
		HttpRequest request;
		std::string buffer = chunkedHeaders() + "5\r\nhello\r\n0\r\n";
		request.feed(buffer);
		check(request.getState() == HttpRequest::CHUNK_TRAILERS && request.getErrorCode() == 0,
		      "Chunked apos '0\\r\\n': estado CHUNK_TRAILERS");
		check(buffer.empty(), "Chunked apos '0\\r\\n': linha de tamanho consumida");

		buffer += "\r";
		request.feed(buffer);
		check(request.getState() == HttpRequest::CHUNK_TRAILERS && request.getErrorCode() == 0,
		      "Chunked trailers: CR final sozinho aguarda");

		buffer += "\n";
		request.feed(buffer);
		expectComplete(request, "Chunked trailers: completa ao receber o LF final");
		check(request.getBody() == "hello", "Chunked trailers: body preservado");
	}
	{
		HttpRequest request;
		std::string buffer = chunkedHeaders() + "0\r\nX-A: b\r\n";
		request.feed(buffer);
		check(request.getState() == HttpRequest::CHUNK_TRAILERS && request.getErrorCode() == 0,
		      "Chunked trailer sem linha em branco: aguarda");
		check(buffer == "X-A: b\r\n", "Chunked trailer sem linha em branco: buffer preservado");

		buffer += "\r\n";
		request.feed(buffer);
		expectComplete(request, "Chunked trailer: completa ao receber a linha em branco");
	}
}

void testChunkedByteByByte() {
	HttpRequest request;
	std::string data = chunkedHeaders() + "5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n";

	check(feedByteByByteStrict(request, data),
	      "Chunked byte a byte: nao completa nem erra antes do ultimo byte");
	expectComplete(request, "Chunked byte a byte: requisicao completa");
	check(request.getBody() == "hello world", "Chunked byte a byte: conteudo do body");
}

void testChunkedExtraBytes() {
	{
		HttpRequest request;
		std::string next = "GET /next HTTP/1.1\r\n";
		std::string buffer = chunkedHeaders() + "5\r\nhello\r\n0\r\n\r\n" + next;
		request.feed(buffer);

		expectComplete(request, "Chunked + proxima requisicao: completa");
		check(request.getBody() == "hello", "Chunked + proxima requisicao: body correto");
		check(buffer == next,               "Chunked + proxima requisicao: bytes seguintes preservados");
	}
	{
		HttpRequest request;
		std::string next = "GET /next HTTP/1.1\r\n";
		std::string buffer = chunkedHeaders() + "0\r\n\r\n" + next;
		request.feed(buffer);

		expectComplete(request, "Chunked vazio + proxima requisicao: completa");
		check(buffer == next, "Chunked vazio + proxima requisicao: bytes seguintes preservados");
	}
	{
		HttpRequest request;
		std::string next = "GET /next HTTP/1.1\r\n";
		std::string buffer = chunkedHeaders() + "0\r\nX-T: v\r\n\r\n" + next;
		request.feed(buffer);

		expectComplete(request, "Chunked com trailer + proxima requisicao: completa");
		check(buffer == next, "Chunked com trailer + proxima requisicao: bytes seguintes preservados");
	}
}

void testChunkedErrors() {
	const ErrCase cases[] = {
		// linha de tamanho
		{ "tamanho nao hexa",             CHUNKED "g\r\nhello\r\n0\r\n\r\n",        STATUS_BAD_REQUEST },
		{ "tamanho negativo",             CHUNKED "-1\r\nhello\r\n0\r\n\r\n",       STATUS_BAD_REQUEST },
		{ "tamanho com sinal +",          CHUNKED "+5\r\nhello\r\n0\r\n\r\n",       STATUS_BAD_REQUEST },
		{ "linha de tamanho vazia",       CHUNKED "\r\nhello\r\n0\r\n\r\n",         STATUS_BAD_REQUEST },
		{ "espaco antes do tamanho",      CHUNKED " 5\r\nhello\r\n0\r\n\r\n",       STATUS_BAD_REQUEST },
		{ "espaco depois (sem extensao)", CHUNKED "5 \r\nhello\r\n0\r\n\r\n",       STATUS_BAD_REQUEST },
		{ "TAB depois (sem extensao)",    CHUNKED "5\t\r\nhello\r\n0\r\n\r\n",      STATUS_BAD_REQUEST },
		{ "prefixo 0x",                   CHUNKED "0x5\r\nhello\r\n0\r\n\r\n",      STATUS_BAD_REQUEST },
		{ "lixo depois do numero",        CHUNKED "5x\r\nhello\r\n0\r\n\r\n",       STATUS_BAD_REQUEST },
		{ "inicia com ';'",               CHUNKED ";a\r\nhello\r\n0\r\n\r\n",       STATUS_BAD_REQUEST },
		{ "LF solto na linha de tamanho", CHUNKED "5\nhello\r\n0\r\n\r\n",          STATUS_BAD_REQUEST },
		{ "CR solto na linha de tamanho", CHUNKED "5\rX\r\nhello\r\n0\r\n\r\n",     STATUS_BAD_REQUEST },

		// extensoes com caracteres proibidos
		{ "extensao com controle",        CHUNKED "5;a\001b\r\nhello\r\n0\r\n\r\n", STATUS_BAD_REQUEST },
		{ "extensao com DEL",             CHUNKED "5;a\177\r\nhello\r\n0\r\n\r\n",  STATUS_BAD_REQUEST },
		{ "extensao com byte alto",       CHUNKED "5;\303\251\r\nhello\r\n0\r\n\r\n", STATUS_BAD_REQUEST },
		{ "extensao com CR solto",        CHUNKED "5;a\r\r\nhello\r\n0\r\n\r\n",    STATUS_BAD_REQUEST },
		{ "chunk final com extensao invalida", CHUNKED "0;a\001\r\n\r\n",           STATUS_BAD_REQUEST },

		// dados e terminador do chunk
		{ "dados mais curtos que o tamanho", CHUNKED "5\r\nhel\r\n0\r\n\r\n",       STATUS_BAD_REQUEST },
		{ "dados mais longos que o tamanho", CHUNKED "2\r\nhello\r\n0\r\n\r\n",     STATUS_BAD_REQUEST },
		{ "sem CRLF apos os dados",       CHUNKED "5\r\nhelloXX0\r\n\r\n",          STATUS_BAD_REQUEST },
		{ "LF sem CR apos os dados",      CHUNKED "5\r\nhello\n0\r\n\r\n",          STATUS_BAD_REQUEST },
		{ "linha em branco entre chunks", CHUNKED "5\r\nhello\r\n\r\n0\r\n\r\n",    STATUS_BAD_REQUEST },
		{ "segunda linha de tamanho invalida", CHUNKED "5\r\nhello\r\nZZ\r\n",     STATUS_BAD_REQUEST },
		{ "dados depois do chunk final (trailer invalido)", CHUNKED "0\r\nhello\r\n\r\n", STATUS_BAD_REQUEST },

		// trailers
		{ "trailer sem dois-pontos",      CHUNKED "0\r\nBadTrailer\r\n\r\n",        STATUS_BAD_REQUEST },
		{ "trailer Content-Length",       CHUNKED "0\r\nContent-Length: 5\r\n\r\n", STATUS_BAD_REQUEST },
		{ "trailer Transfer-Encoding",    CHUNKED "0\r\nTransfer-Encoding: chunked\r\n\r\n", STATUS_BAD_REQUEST },
		{ "trailer com espaco antes do ':'", CHUNKED "0\r\nX : v\r\n\r\n",          STATUS_BAD_REQUEST },
		{ "trailer sem nome",             CHUNKED "0\r\n: v\r\n\r\n",               STATUS_BAD_REQUEST },
		{ "trailer com nome invalido",    CHUNKED "0\r\nBad Name: v\r\n\r\n",       STATUS_BAD_REQUEST },
		{ "trailer com obs-fold",         CHUNKED "0\r\nX: a\r\n b: c\r\n\r\n",     STATUS_BAD_REQUEST },
		{ "trailer com valor invalido",   CHUNKED "0\r\nX: a\001b\r\n\r\n",         STATUS_BAD_REQUEST },
		{ "trailer com CR solto",         CHUNKED "0\r\nX: a\rb\r\n\r\n",           STATUS_BAD_REQUEST },
	};
	runErrCases("Chunked", cases, COUNT(cases));
}

void testChunkedLimitErrors() {
	const ErrCase cases[] = {
		{ "um chunk de 11 bytes",         CHUNKED "B\r\nhello world\r\n0\r\n\r\n",  STATUS_CONTENT_TOO_LARGE },
		{ "recusa antes de receber os dados", CHUNKED "B\r\n",                       STATUS_CONTENT_TOO_LARGE },
		{ "soma dos chunks (12) acima",   CHUNKED "6\r\nabcdef\r\n6\r\nghijkl\r\n0\r\n\r\n", STATUS_CONTENT_TOO_LARGE },
		{ "segundo chunk gigante",        CHUNKED "5\r\nhello\r\nFFFFFFFFFFFFFFFF\r\n", hugeChunkCode() },
		{ "segundo chunk estoura o saldo", CHUNKED "5\r\nhello\r\n6\r\n",            STATUS_CONTENT_TOO_LARGE },
	};
	runErrCases("Chunked com limite 10", cases, COUNT(cases), 10);

	HttpRequest request(10);
	feedAll(request, chunkedHeaders() + "6\r\nabcdef\r\n4\r\nghij\r\n0\r\n\r\n");
	expectComplete(request, "Chunked soma 10 bytes com limite 10: aceito");
	check(request.getBody() == "abcdefghij", "Chunked soma 10 bytes com limite 10: conteudo do body");
}

void testChunkedSizeLineLimits() {
	const std::string hello = "\r\nhello\r\n0\r\n\r\n";

	const int lengths[] = { 100, static_cast<int>(kHuge) };
	for (size_t i = 0; i < COUNT(lengths); ++i) {
		std::string digits(lengths[i], '1');
		{
			HttpRequest request;
			feedAll(request, chunkedHeaders() + digits);
			expectError(request, STATUS_BAD_REQUEST,
			            "Chunked linha de tamanho de " + num(lengths[i]) + " bytes sem CRLF: erro");
		}
		{
			HttpRequest request;
			feedAll(request, chunkedHeaders() + digits + hello);
			expectError(request, STATUS_BAD_REQUEST,
			            "Chunked linha de tamanho de " + num(lengths[i]) + " bytes com CRLF: erro");
		}
	}
	{
		HttpRequest request;
		feedAll(request, chunkedHeaders() + "5;" + std::string(kHuge, 'a') + hello);
		expectError(request, STATUS_BAD_REQUEST, "Chunked extensao gigante com CRLF: erro");
	}
	{
		HttpRequest request;
		feedAll(request, chunkedHeaders() + "5;" + std::string(kHuge, 'a'));
		expectError(request, STATUS_BAD_REQUEST, "Chunked extensao gigante sem CRLF: erro imediato");
	}
	{
		// Mesmo erro se a linha chegar aos poucos.
		HttpRequest request;
		feedByByte(request, chunkedHeaders() + "5;" + std::string(kHuge, 'a') + hello);
		expectError(request, STATUS_BAD_REQUEST, "Chunked extensao gigante byte a byte: erro");
	}
}

void testChunkedSizeOverflow() {
	// 20 digitos F nao cabem em size_t (64 bits): nao pode quebrar o parser.
	HttpRequest request;
	feedAll(request, chunkedHeaders() + std::string(20, 'F') + "\r\nhello\r\n0\r\n\r\n");

	check(request.getState() == HttpRequest::REQUEST_ERROR &&
	      (request.getErrorCode() == STATUS_BAD_REQUEST ||
	       request.getErrorCode() == STATUS_CONTENT_TOO_LARGE),
	      "Chunked tamanho com overflow: erro apropriado");

	HttpRequest request2;
	feedAll(request2, chunkedHeaders() + "FFFFFFFFFFFFFFFF\r\n");
	expectError(request2, hugeChunkCode(), "Chunked tamanho SIZE_MAX: STATUS_CONTENT_TOO_LARGE");
}

void testChunkedTrailerSizeLimits() {
	{
		HttpRequest request;
		feedAll(request, chunkedHeaders() + "0\r\nX: " + std::string(kMaxHeadersSize + 800, 'a') + "\r\n\r\n");
		expectError(request, STATUS_HEADERS_TOO_LARGE, "Chunked trailer gigante: STATUS_HEADERS_TOO_LARGE");
	}
	{
		HttpRequest request;
		feedAll(request, chunkedHeaders() + "0\r\nX: " + std::string(kMaxHeadersSize + 800, 'a'));
		expectError(request, STATUS_HEADERS_TOO_LARGE,
		            "Chunked trailer gigante sem terminador: erro imediato");
	}
	{
		HttpRequest request;
		feedAll(request, chunkedHeaders() + "0\r\nX: " + std::string(1000, 'a'));
		check(request.getState() == HttpRequest::CHUNK_TRAILERS && request.getErrorCode() == 0,
		      "Chunked trailer pequeno sem terminador: continua aguardando");
	}
}

void testChunkedManySmallChunks() {
	std::string payload;
	std::string expected;
	for (int i = 0; i < 1000; ++i) {
		payload += "1\r\nX\r\n";
		expected += 'X';
	}
	payload += "0\r\n\r\n";

	HttpRequest request;
	feedAll(request, chunkedHeaders() + payload);

	expectComplete(request, "Chunked 1000 chunks de 1 byte: completo");
	check(request.getBody() == expected, "Chunked 1000 chunks de 1 byte: conteudo");
}

void testChunkedLargeInPieces() {
	std::string data;
	for (size_t i = 0; i < 100000; ++i)
		data += static_cast<char>(i % 253);

	HttpRequest request;
	std::string buffer = chunkedHeaders();
	request.feed(buffer);

	std::string payload = "186A0\r\n" + data + "\r\n0\r\n\r\n";   // 0x186A0 = 100000
	for (size_t pos = 0; pos < payload.size(); pos += 997) {
		buffer.append(payload, pos, 997);
		request.feed(buffer);
	}

	expectComplete(request, "Chunked de 100000 bytes em pedacos de 997: completo");
	check(request.getBody() == data, "Chunked de 100000 bytes: conteudo identico");
	check(buffer.empty(),            "Chunked de 100000 bytes: buffer consumido");
}

// ===========================================================================
// Bytes NUL (nao podem truncar nem enganar o parser)
// ===========================================================================

void testNulBytes() {
	const std::string nul(1, '\0');
	{
		HttpRequest request;
		feedAll(request, "GET /a" + nul + "b HTTP/1.1\r\n");
		expectError(request, STATUS_BAD_REQUEST, "NUL na URI: STATUS_BAD_REQUEST");
	}
	{
		HttpRequest request;
		feedAll(request, "GET /a?q=" + nul + " HTTP/1.1\r\n");
		expectError(request, STATUS_BAD_REQUEST, "NUL na query: STATUS_BAD_REQUEST");
	}
	{
		HttpRequest request;
		feedAll(request, "G" + nul + "T / HTTP/1.1\r\n");
		expectError(request, STATUS_BAD_REQUEST, "NUL no metodo: STATUS_BAD_REQUEST");
	}
	{
		HttpRequest request;
		feedAll(request, GETH "X-A: a" + nul + "b\r\n\r\n");
		expectError(request, STATUS_BAD_REQUEST, "NUL no valor de header: STATUS_BAD_REQUEST");
	}
	{
		HttpRequest request;
		feedAll(request, GETH "X-" + nul + "A: v\r\n\r\n");
		expectError(request, STATUS_BAD_REQUEST, "NUL no nome de header: STATUS_BAD_REQUEST");
	}
	{
		HttpRequest request;
		feedAll(request, std::string(REQLINE "Host: local") + nul + "host\r\n\r\n");
		expectError(request, STATUS_BAD_REQUEST, "NUL no Host: STATUS_BAD_REQUEST");
	}
	{
		HttpRequest request;
		feedAll(request, GETH "Content-Length: 5" + nul + "\r\n\r\n");
		expectError(request, STATUS_BAD_REQUEST, "NUL no Content-Length: STATUS_BAD_REQUEST");
	}
	{
		HttpRequest request;
		feedAll(request, chunkedHeaders() + "5" + nul + "\r\nhello\r\n0\r\n\r\n");
		expectError(request, STATUS_BAD_REQUEST, "NUL na linha de tamanho do chunk: STATUS_BAD_REQUEST");
	}
	{
		HttpRequest request;
		feedAll(request, chunkedHeaders() + "5;a" + nul + "\r\nhello\r\n0\r\n\r\n");
		expectError(request, STATUS_BAD_REQUEST, "NUL na extensao do chunk: STATUS_BAD_REQUEST");
	}
	{
		HttpRequest request;
		feedAll(request, chunkedHeaders() + "0\r\nX: a" + nul + "b\r\n\r\n");
		expectError(request, STATUS_BAD_REQUEST, "NUL no valor do trailer: STATUS_BAD_REQUEST");
	}
	{
		// NUL nos dados do chunk e valido.
		std::string data("a\0b\0c", 5);
		HttpRequest request;
		feedAll(request, chunkedHeaders() + "5\r\n" + data + "\r\n0\r\n\r\n");
		expectComplete(request, "NUL nos dados do chunk: aceito");
		check(request.getBody() == data, "NUL nos dados do chunk: preservado");
	}
}

// ===========================================================================
// Estado terminal e pipelining
// ===========================================================================

void testCompleteIsSticky() {
	HttpRequest request;
	std::string buffer = GETH "\r\n";
	request.feed(buffer);
	expectComplete(request, "Estado terminal: requisicao completa");

	std::string next = "GET /other HTTP/1.1\r\n";
	buffer = next;
	request.feed(buffer);

	expectComplete(request, "Estado terminal: continua COMPLETE ao receber mais dados");
	check(buffer == next,       "Estado terminal: nao consome dados depois de completa");
	check(request.getPath() == "/", "Estado terminal: path original preservado");
}

void testErrorIsSticky() {
	HttpRequest request;
	std::string buffer = "FOO / HTTP/1.1\r\n";
	request.feed(buffer);
	expectError(request, STATUS_NOT_IMPLEMENTED, "Erro: primeira falha detectada");

	std::string next = GETH "\r\n";
	buffer = next;
	request.feed(buffer);

	expectError(request, STATUS_NOT_IMPLEMENTED, "Erro: continua em erro e mantem o primeiro codigo");
	check(buffer == next, "Erro: nao consome dados depois do erro");
}

void testPipelinedRequests() {
	{
		std::string buffer = GETH "\r\n" + lengthHeaders(5) + "hello" + GETH "\r\n";

		HttpRequest first;
		first.feed(buffer);
		expectComplete(first, "Pipelining 1/3 (GET): completa");

		HttpRequest second;
		second.feed(buffer);
		expectComplete(second, "Pipelining 2/3 (POST com Content-Length): completa");
		check(second.getBody() == "hello", "Pipelining 2/3: body correto");

		HttpRequest third;
		third.feed(buffer);
		expectComplete(third, "Pipelining 3/3 (GET): completa");
		check(buffer.empty(), "Pipelining: buffer totalmente consumido");
	}
	{
		std::string buffer = chunkedHeaders() + "5\r\nhello\r\n0\r\nX-T: v\r\n\r\n" +
		                     chunkedHeaders() + "3\r\nabc\r\n0\r\n\r\n" + GETH "\r\n";

		HttpRequest first;
		first.feed(buffer);
		expectComplete(first, "Pipelining chunked 1/3: completa");
		check(first.getBody() == "hello", "Pipelining chunked 1/3: body correto");

		HttpRequest second;
		second.feed(buffer);
		expectComplete(second, "Pipelining chunked 2/3: completa");
		check(second.getBody() == "abc", "Pipelining chunked 2/3: body correto");

		HttpRequest third;
		third.feed(buffer);
		expectComplete(third, "Pipelining chunked 3/3 (GET): completa");
		check(buffer.empty(), "Pipelining chunked: buffer totalmente consumido");
	}
}

// ===========================================================================
// Fragmentacao: qualquer corte tem que dar o mesmo resultado
// ===========================================================================

std::string manyHeadersRequest() {
	std::string s = GETH;
	for (int i = 0; i < 30; ++i)
		s += "X-Test-" + num(i) + ": value" + num(i) + "\r\n";
	return s + "Connection: close\r\n\r\n";
}

void testFragmentation() {
	const std::string pipelineTail = "GET /next HTTP/1.1\r\n";

	checkSplitEquivalence("Fragmentacao GET simples",
		std::string("GET /a/b?x=1 HTTP/1.1\r\nHost: localhost\r\nAccept: */*\r\n\r\n") + pipelineTail);

	checkSplitEquivalence("Fragmentacao POST com Content-Length",
		lengthHeaders(11) + "hello world" + pipelineTail);

	checkSplitEquivalence("Fragmentacao POST chunked com extensao e trailer",
		chunkedHeaders() + "5;a=b\r\nhello\r\n6\r\n world\r\n0;x\r\nT: v\r\n\r\n" + pipelineTail);

	checkSplitEquivalence("Fragmentacao com linhas vazias iniciais",
		"\r\n\r\nGET / HTTP/1.1\r\nHost: localhost\r\n\r\n");

	checkRandomSplits("POST chunked grande",
		chunkedHeaders() + "A\r\n0123456789\r\n1F\r\n" + std::string(31, 'x') +
		"\r\n5;ext=1\r\nhello\r\n0\r\nT1: a\r\nT2: b\r\n\r\n" + pipelineTail);

	checkRandomSplits("POST com Content-Length",
		lengthHeaders(300) + std::string(300, 'z') + pipelineTail);

	checkRandomSplits("GET com muitos headers", manyHeadersRequest());

	checkRandomSplits("Erro de chunk no meio do body",
		chunkedHeaders() + "5\r\nhello\r\n3\r\nabXX0\r\n\r\n");

	checkRandomSplits("Erro de trailer",
		chunkedHeaders() + "5\r\nhello\r\n0\r\nContent-Length: 5\r\n\r\n");

	checkRandomSplits("Chunked acima do limite", chunkedHeaders() + "6\r\nabcdef\r\n6\r\nghijkl\r\n0\r\n\r\n", 10);
}

// ===========================================================================

