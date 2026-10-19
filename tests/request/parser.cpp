
#include "HttpRequest/HttpRequest.hpp"
#include "StatusCode/StatusCode.hpp"

#include <iostream>
#include <string>
#include <sstream>

static int testsPassed = 0;
static int testsFailed = 0;

static void check(bool condition, const std::string& name)
{
    if (condition) {
        ++testsPassed;
        std::cout << "[PASS] " << name << "\n";
    } else {
        ++testsFailed;
        std::cout << "[FAIL] " << name << "\n";
    }
}

static void testValidRequest()
{
    HttpRequest request;
    std::string buffer = "GET /index.html HTTP/1.1\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::PARSING_HEADERS,
          "GET valido: estado PARSING_HEADERS");

    check(request.getErrorCode() == 0,
          "GET valido: sem erro");

    check(request.getMethod() == "GET",
          "GET valido: metodo");

    check(request.getUri() == "/index.html",
          "GET valido: URI");

    check(request.getPath() == "/index.html",
          "GET valido: path");

    check(request.getHttpVersion() == "HTTP/1.1",
          "GET valido: versao HTTP");

    check(buffer.empty(),
          "GET valido: buffer consumido");
}

static void testQueryString()
{
    HttpRequest request;
    std::string buffer =
        "GET /search?q=cpp HTTP/1.1\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::PARSING_HEADERS,
          "Query: estado correto");

    check(request.getUri() == "/search?q=cpp",
          "Query: URI original");

    check(request.getPath() == "/search",
          "Query: path separado");

    check(request.getQueryString() == "q=cpp",
          "Query: string separada");
}

static void testSupportedMethods()
{
    const char* methods[] = {"GET", "POST", "DELETE"};

    for (int i = 0; i < 3; ++i) {
        HttpRequest request;
        std::string buffer =
            std::string(methods[i]) + " / HTTP/1.1\r\n";

        request.feed(buffer);

        check(request.getState() == HttpRequest::PARSING_HEADERS,
              std::string("Metodo suportado: ") + methods[i]);

        check(request.getMethod() == methods[i],
              std::string("Metodo armazenado: ") + methods[i]);
    }
}

static void testMalformedRequest()
{
    HttpRequest request;
    std::string buffer = "GET  /index.html HTTP/1.1\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Espacos extras: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Espacos extras: STATUS_BAD_REQUEST");
}

static void testUnknownMethod()
{
    HttpRequest request;
    std::string buffer = "FOO / HTTP/1.1\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Metodo desconhecido: estado de erro");

    check(request.getErrorCode() == STATUS_NOT_IMPLEMENTED,
          "Metodo desconhecido: STATUS_NOT_IMPLEMENTED");
}

static void testInvalidUri()
{
    HttpRequest request;
    std::string buffer = "GET /%GG HTTP/1.1\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Escape hexadecimal invalido: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Escape hexadecimal invalido: STATUS_BAD_REQUEST");
}

static void testPathTraversal()
{
    HttpRequest request;
    std::string buffer = "GET /../secret HTTP/1.1\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Path traversal: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Path traversal: STATUS_BAD_REQUEST");
}

static void testUnsupportedVersion()
{
    HttpRequest request;
    std::string buffer = "GET / HTTP/2.0\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "HTTP/2.0: estado de erro");

    check(request.getErrorCode() == STATUS_VERSION_NOT_SUPPORTED,
          "HTTP/2.0: versao nao suportada");
}

static void testIncompleteLine()
{
    HttpRequest request;
    std::string buffer = "GET / HTTP/1.1";

    request.feed(buffer);

    check(request.getState() == HttpRequest::PARSING_REQUEST_LINE,
          "Linha incompleta: continua aguardando");

    check(request.getErrorCode() == 0,
          "Linha incompleta: sem erro");

    check(buffer == "GET / HTTP/1.1",
          "Linha incompleta: buffer preservado");
}

static void testAdditionalData()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::PARSING_HEADERS,
          "Dados adicionais: primeira linha aceita");

    check(buffer == "Host: localhost\r\n",
          "Dados adicionais: restante preservado");
}

static void testEmptyLineBeforeRequest()
{
    HttpRequest request;
    std::string buffer = "\r\nGET / HTTP/1.1\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::PARSING_HEADERS,
          "Linha vazia inicial: requisicao aceita");

    check(request.getMethod() == "GET",
          "Linha vazia inicial: metodo correto");
}

static void testLongRequestLine()
{
    HttpRequest request;

    // O metodo, os espacos e a versao tambem contam no comprimento.
    std::string buffer = "GET /";

    while (buffer.size() < 8193 - 1) {
        buffer += 'a';
    }

    buffer += " HTTP/1.1\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Linha longa: estado de erro");

    check(request.getErrorCode() == STATUS_URI_TOO_LONG,
          "Linha longa: STATUS_URI_TOO_LONG");
}


static void testValidHeaders()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "User-Agent: WebservTest/1.0\r\n"
        "Accept: */*\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Headers validos: requisicao completa");

    check(request.getErrorCode() == 0,
          "Headers validos: sem erro");

    check(buffer.empty(),
          "Headers validos: buffer consumido");
}

static void testMissingHost()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "User-Agent: WebservTest/1.0\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Host ausente: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Host ausente: STATUS_BAD_REQUEST");
}

static void testEmptyHost()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: \r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Host vazio: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Host vazio: STATUS_BAD_REQUEST");
}

static void testDuplicateHost()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Host: example.com\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Host duplicado: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Host duplicado: STATUS_BAD_REQUEST");
}

static void testHeaderWithoutColon()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "InvalidHeader\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Header sem dois-pontos: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Header sem dois-pontos: STATUS_BAD_REQUEST");
}

static void testInvalidHeaderName()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Bad Header: value\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Nome de header invalido: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Nome de header invalido: STATUS_BAD_REQUEST");
}

static void testDuplicateContentLength()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 0\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Content-Length duplicado: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Content-Length duplicado: STATUS_BAD_REQUEST");
}

static void testDuplicateTransferEncoding()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Transfer-Encoding duplicado: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Transfer-Encoding duplicado: STATUS_BAD_REQUEST");
}

static void testInvalidContentLength()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: abc\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Content-Length invalido: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Content-Length invalido: STATUS_BAD_REQUEST");
}

static void testNegativeContentLength()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: -1\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Content-Length negativo: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Content-Length negativo: STATUS_BAD_REQUEST");
}

static void testZeroContentLength()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Content-Length zero: requisicao completa");

    check(request.getErrorCode() == 0,
          "Content-Length zero: sem erro");
}

static void testOversizedContentLength()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 1048577\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Content-Length acima do limite: estado de erro");

    check(request.getErrorCode() == STATUS_PAYLOAD_TOO_LARGE,
          "Content-Length acima do limite: STATUS_PAYLOAD_TOO_LARGE");
}

static void testUnsupportedTransferEncoding()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: gzip\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Transfer-Encoding nao suportado: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Transfer-Encoding nao suportado: STATUS_BAD_REQUEST");
}

static void testConflictingBodyHeaders()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Transfer-Encoding com Content-Length: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Transfer-Encoding com Content-Length: STATUS_BAD_REQUEST");
}

static void testDuplicateRegularHeader()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Accept: text/html\r\n"
        "Accept: application/json\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Header comum duplicado: requisicao aceita");

    check(request.getErrorCode() == 0,
          "Header comum duplicado: sem erro");
}

static void testTooManyHeaders()
{
    HttpRequest request;
    std::string buffer = "GET / HTTP/1.1\r\n";
    buffer += "Host: localhost\r\n";

    // Host + 100 headers adicionais = 101 headers.
    for (int i = 0; i < 100; ++i) {
        std::ostringstream name;
        name << "X-Test-" << i;
        buffer += name.str() + ": value\r\n";
    }

    buffer += "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Excesso de headers: estado de erro");

    check(request.getErrorCode() == STATUS_HEADERS_TOO_LARGE,
          "Excesso de headers: STATUS_HEADERS_TOO_LARGE");
}

static void testHeadersTooLarge()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "X-Fill: ";

    // Ultrapassa MAX_HEADERS_SIZE de 8192 bytes.
    buffer.append(8200, 'a');
    buffer += "\r\n\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Headers muito grandes: estado de erro");

    check(request.getErrorCode() == STATUS_HEADERS_TOO_LARGE,
          "Headers muito grandes: STATUS_HEADERS_TOO_LARGE");
}


static void testCaseInsensitiveHeaders()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "hOsT: localhost\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Header case-insensitive: requisicao completa");

    check(request.getErrorCode() == 0,
          "Header case-insensitive: sem erro");
}

static void testInvalidHeaderValue()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "X-Test: value";

    buffer += static_cast<char>(1);
    buffer += "\r\n\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Valor de header com controle: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Valor de header com controle: STATUS_BAD_REQUEST");
}

static void testContentLengthOverflow()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: ";

    // Valor decimal muito maior que qualquer size_t convencional.
    buffer.append(100, '9');
    buffer += "\r\n\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Content-Length enorme: estado de erro");

    check(request.getErrorCode() == STATUS_BAD_REQUEST ||
          request.getErrorCode() == STATUS_PAYLOAD_TOO_LARGE,
          "Content-Length enorme: erro apropriado");
}

static void testContentLengthBodyState()
{
    HttpRequest request;
    std::string buffer =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 10\r\n"
        "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::PARSING_BODY,
          "Content-Length positivo: estado PARSING_BODY");

    check(request.getErrorCode() == 0,
          "Content-Length positivo: sem erro");

    check(buffer.empty(),
          "Content-Length positivo: headers consumidos");
}

static void testMaximumHeaderCount()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n";

    // Total: 1 Host + 99 headers adicionais = 100.
    for (int i = 0; i < 99; ++i) {
        std::ostringstream name;
        name << "X-Test-" << i;
        buffer += name.str() + ": value\r\n";
    }

    buffer += "\r\n";

    request.feed(buffer);

    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "100 headers: requisicao aceita");

    check(request.getErrorCode() == 0,
          "100 headers: sem erro");
}

// ===========================================================================
// Helpers para os testes de limite de body e de body
// ===========================================================================
 
static std::string lengthHeaders(size_t length)
{
    std::ostringstream oss;
    oss << "POST /upload HTTP/1.1\r\n"
        << "Host: localhost\r\n"
        << "Content-Length: " << length << "\r\n"
        << "\r\n";
    return oss.str();
}
 
static std::string chunkedHeaders()
{
    return "POST /upload HTTP/1.1\r\n"
           "Host: localhost\r\n"
           "Transfer-Encoding: chunked\r\n"
           "\r\n";
}
 
// Alimenta a requisicao byte a byte. Devolve false se o estado virou
// REQUEST_COMPLETE ou REQUEST_ERROR antes do ultimo byte.
static bool feedByteByByte(HttpRequest& request, const std::string& data)
{
    std::string buffer;
 
    for (size_t i = 0; i < data.size(); ++i) {
        buffer += data[i];
        request.feed(buffer);
 
        bool last = (i + 1 == data.size());
        if (!last &&
            (request.getState() == HttpRequest::REQUEST_COMPLETE ||
             request.getState() == HttpRequest::REQUEST_ERROR))
            return false;
    }
    return true;
}
 
// ===========================================================================
// Headers: normalizacao e Transfer-Encoding
// ===========================================================================
 
static void testHeaderValueNormalization()
{
    HttpRequest request;
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host:   localhost:8080  \r\n"
        "X-Mixed:\tAbC  dEf\t\r\n"
        "\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Normalizacao: requisicao completa");
 
    check(request.getHeader("host") == "localhost:8080",
          "Normalizacao: espacos das pontas removidos, ':' do valor mantido");
 
    check(request.getHeader("x-mixed") == "AbC  dEf",
          "Normalizacao: nome em minusculas, valor com maiusculas e espacos internos preservados");
 
    check(request.hasHeader("x-mixed") && !request.hasHeader("X-Mixed"),
          "Normalizacao: chaves guardadas em minusculas");
}
 
static void testTransferEncodingCaseInsensitive()
{
    // RFC 9112, secao 7: nomes de transfer coding nao diferenciam maiusculas.
    HttpRequest request;
    std::string buffer =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: Chunked\r\n"
        "\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::PARSING_BODY,
          "Transfer-Encoding: Chunked (maiusculo) aceito");
 
    check(request.getErrorCode() == 0,
          "Transfer-Encoding: Chunked (maiusculo) sem erro");
}
 
static void testChunkedNotFinalCoding()
{
    HttpRequest request;
    std::string buffer =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked, gzip\r\n"
        "\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "chunked nao e o ultimo coding: estado de erro");
 
    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "chunked nao e o ultimo coding: STATUS_BAD_REQUEST");
}
 
// ===========================================================================
// Limite de body (max_body vindo do ServerConfig via construtor)
// ===========================================================================
 
static void testDefaultLimitBoundary()
{
    HttpRequest request;
    std::string buffer = lengthHeaders(1048576);
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::PARSING_BODY,
          "Limite padrao: Content-Length igual ao limite (1 MB) aceito");
 
    check(request.getErrorCode() == 0,
          "Limite padrao: Content-Length igual ao limite sem erro");
}
 
static void testCustomLimitAcceptsEqual()
{
    HttpRequest request(10);
    std::string buffer = lengthHeaders(10);
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::PARSING_BODY,
          "Limite 10: Content-Length 10 aceito");
 
    check(request.getErrorCode() == 0,
          "Limite 10: Content-Length 10 sem erro");
}
 
static void testCustomLimitRejectsAbove()
{
    HttpRequest request(10);
    std::string buffer = lengthHeaders(11);
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Limite 10: Content-Length 11 estado de erro");
 
    check(request.getErrorCode() == STATUS_PAYLOAD_TOO_LARGE,
          "Limite 10: Content-Length 11 STATUS_PAYLOAD_TOO_LARGE");
}
 
static void testCustomLimitAboveDefault()
{
    // Pega o erro de continuar comparando com o limite padrao (1 MB)
    // em vez do limite recebido no construtor.
    HttpRequest request(10 * 1024 * 1024);
    std::string buffer = lengthHeaders(5 * 1024 * 1024);
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::PARSING_BODY,
          "Limite 10 MB: Content-Length 5 MB aceito");
 
    check(request.getErrorCode() == 0,
          "Limite 10 MB: Content-Length 5 MB sem erro");
}
 
static void testCustomLimitContentLengthOverflow()
{
    HttpRequest request(10);
    std::string buffer =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: ";
 
    buffer.append(100, '9');
    buffer += "\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Limite 10: Content-Length gigante estado de erro");
 
    check(request.getErrorCode() == STATUS_PAYLOAD_TOO_LARGE,
          "Limite 10: Content-Length gigante STATUS_PAYLOAD_TOO_LARGE");
}
 
// ===========================================================================
// Body com Content-Length
// ===========================================================================
 
static void testBodyContentLengthComplete()
{
    HttpRequest request;
    std::string buffer = lengthHeaders(5) + "hello";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Body CL: requisicao completa");
 
    check(request.getErrorCode() == 0,
          "Body CL: sem erro");
 
    check(request.getBody() == "hello",
          "Body CL: conteudo do body");
 
    check(buffer.empty(),
          "Body CL: buffer consumido");
}
 
static void testBodyContentLengthHeadersThenBody()
{
    HttpRequest request;
    std::string buffer = lengthHeaders(5);
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::PARSING_BODY,
          "Body CL separado: apos os headers espera o body");
 
    buffer += "hello";
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Body CL separado: completa quando o body chega");
 
    check(request.getBody() == "hello",
          "Body CL separado: conteudo do body");
}
 
static void testBodyContentLengthInPieces()
{
    HttpRequest request;
    std::string buffer = lengthHeaders(10) + "hel";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::PARSING_BODY,
          "Body CL em partes: body parcial continua aguardando");
 
    check(request.getErrorCode() == 0,
          "Body CL em partes: body parcial sem erro");
 
    buffer += "lo wo";
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::PARSING_BODY,
          "Body CL em partes: ainda faltam bytes");
 
    buffer += "rld";   // so 2 bytes sao necessarios; 1 byte a mais fica no buffer
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Body CL em partes: completa ao receber todos os bytes");
 
    check(request.getBody() == "hello worl",
          "Body CL em partes: body tem exatamente Content-Length bytes");
 
    check(buffer == "d",
          "Body CL em partes: byte excedente fica no buffer");
}
 
static void testBodyContentLengthByteByByte()
{
    HttpRequest request;
    std::string data = lengthHeaders(11) + "hello world";
 
    check(feedByteByByte(request, data),
          "Body CL byte a byte: nao completa nem erra antes do ultimo byte");
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Body CL byte a byte: requisicao completa");
 
    check(request.getBody() == "hello world",
          "Body CL byte a byte: conteudo do body");
}
 
static void testBodyContentLengthExtraBytes()
{
    HttpRequest request;
    std::string next = "GET /next HTTP/1.1\r\n";
    std::string buffer = lengthHeaders(5) + "hello" + next;
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Body CL + proxima requisicao: completa");
 
    check(request.getBody() == "hello",
          "Body CL + proxima requisicao: body com exatamente 5 bytes");
 
    check(buffer == next,
          "Body CL + proxima requisicao: bytes seguintes preservados no buffer");
}
 
static void testBodyContentLengthBinary()
{
    // O body pode ter CRLF, CRLFCRLF e byte nulo: nada disso e delimitador.
    std::string body = "ab\r\n\r\ncd";
    body += '\0';
    body += "ef";
 
    HttpRequest request;
    std::string buffer = lengthHeaders(body.size()) + body;
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Body CL binario: requisicao completa");
 
    check(request.getBody() == body,
          "Body CL binario: CRLF e byte nulo preservados");
}
 
static void testBodyContentLengthMissingBytes()
{
    HttpRequest request;
    std::string buffer = lengthHeaders(10) + "abcd";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::PARSING_BODY,
          "Body CL incompleto: continua em PARSING_BODY");
 
    check(request.getErrorCode() == 0,
          "Body CL incompleto: sem erro");
}
 
static void testNoBodyKeepsExtraBytes()
{
    // Sem Content-Length nem Transfer-Encoding o body e vazio
    // (RFC 9112, secao 6.3): o que vier depois dos headers e outra requisicao.
    HttpRequest request;
    std::string next = "GET /next HTTP/1.1\r\n";
    std::string buffer =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n" + next;
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Sem body: requisicao completa nos headers");
 
    check(request.getBody().empty(),
          "Sem body: body vazio");
 
    check(buffer == next,
          "Sem body: bytes seguintes nao sao consumidos como body");
}
 
static void testZeroContentLengthKeepsExtraBytes()
{
    HttpRequest request;
    std::string next = "GET /next HTTP/1.1\r\n";
    std::string buffer = lengthHeaders(0) + next;
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Content-Length 0 + proxima requisicao: completa");
 
    check(request.getBody().empty(),
          "Content-Length 0 + proxima requisicao: body vazio");
 
    check(buffer == next,
          "Content-Length 0 + proxima requisicao: bytes seguintes preservados");
}
 
// ===========================================================================
// Body chunked
// ===========================================================================
 
static void testChunkedSingleChunk()
{
    HttpRequest request;
    std::string buffer = chunkedHeaders() + "5\r\nhello\r\n0\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Chunked 1 chunk: requisicao completa");
 
    check(request.getErrorCode() == 0,
          "Chunked 1 chunk: sem erro");
 
    check(request.getBody() == "hello",
          "Chunked 1 chunk: conteudo do body");
 
    check(buffer.empty(),
          "Chunked 1 chunk: buffer consumido");
}
 
static void testChunkedMultipleChunks()
{
    HttpRequest request;
    std::string buffer =
        chunkedHeaders() + "5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Chunked varios chunks: requisicao completa");
 
    check(request.getBody() == "hello world",
          "Chunked varios chunks: chunks concatenados na ordem");
}
 
static void testChunkedHexSizes()
{
    {
        HttpRequest request;
        std::string buffer = chunkedHeaders() + "a\r\n0123456789\r\n0\r\n\r\n";
 
        request.feed(buffer);
 
        check(request.getState() == HttpRequest::REQUEST_COMPLETE &&
              request.getBody() == "0123456789",
              "Chunked: tamanho em hexa minusculo (a)");
    }
    {
        HttpRequest request;
        std::string buffer = chunkedHeaders() + "A\r\n0123456789\r\n0\r\n\r\n";
 
        request.feed(buffer);
 
        check(request.getState() == HttpRequest::REQUEST_COMPLETE &&
              request.getBody() == "0123456789",
              "Chunked: tamanho em hexa maiusculo (A)");
    }
    {
        HttpRequest request;
        std::string data(31, 'x');
        std::string buffer = chunkedHeaders() + "1F\r\n" + data + "\r\n0\r\n\r\n";
 
        request.feed(buffer);
 
        check(request.getState() == HttpRequest::REQUEST_COMPLETE &&
              request.getBody() == data,
              "Chunked: tamanho com dois digitos hexa (1F = 31 bytes)");
    }
}
 
static void testChunkedLeadingZeros()
{
    HttpRequest request;
    std::string buffer = chunkedHeaders() + "005\r\nhello\r\n0\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Chunked: tamanho com zeros a esquerda aceito");
 
    check(request.getBody() == "hello",
          "Chunked: zeros a esquerda nao alteram o conteudo");
}
 
static void testChunkedExtensionIgnored()
{
    // RFC 9112, secao 7.1.1: extensoes de chunk desconhecidas sao ignoradas.
    HttpRequest request;
    std::string buffer =
        chunkedHeaders() + "5;name=value\r\nhello\r\n0\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Chunked: extensao de chunk ignorada");
 
    check(request.getBody() == "hello",
          "Chunked: extensao de chunk nao entra no body");
}
 
static void testChunkedTrailersIgnored()
{
    HttpRequest request;
    std::string buffer =
        chunkedHeaders() + "5\r\nhello\r\n0\r\nX-Trailer: value\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Chunked: trailer aceito e requisicao completa");
 
    check(request.getBody() == "hello",
          "Chunked: trailer nao entra no body");
 
    check(buffer.empty(),
          "Chunked: trailer consumido do buffer");
}
 
static void testChunkedDataWithCrlf()
{
    // O tamanho do chunk manda: CRLF dentro dos dados nao encerra o chunk.
    HttpRequest request;
    std::string data = "ab\r\n\r\ncd";
    std::string buffer = chunkedHeaders() + "8\r\n" + data + "\r\n0\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Chunked com CRLF nos dados: requisicao completa");
 
    check(request.getBody() == data,
          "Chunked com CRLF nos dados: dados preservados");
}
 
static void testChunkedExtraBytes()
{
    HttpRequest request;
    std::string next = "GET /next HTTP/1.1\r\n";
    std::string buffer = chunkedHeaders() + "5\r\nhello\r\n0\r\n\r\n" + next;
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Chunked + proxima requisicao: completa");
 
    check(request.getBody() == "hello",
          "Chunked + proxima requisicao: body correto");
 
    check(buffer == next,
          "Chunked + proxima requisicao: bytes seguintes preservados no buffer");
}
 
static void testChunkedHeadersThenBody()
{
    HttpRequest request;
    std::string buffer = chunkedHeaders();
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::PARSING_BODY,
          "Chunked separado: apos os headers espera o body");
 
    buffer += "5\r\nhello\r\n0\r\n\r\n";
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Chunked separado: completa quando os chunks chegam");
 
    check(request.getBody() == "hello",
          "Chunked separado: conteudo do body");
}
 
static void testChunkedDataInPieces()
{
    HttpRequest request;
    std::string buffer = chunkedHeaders() + "5\r\nhel";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::PARSING_BODY,
          "Chunked dados parciais: continua aguardando");
 
    check(request.getErrorCode() == 0,
          "Chunked dados parciais: sem erro");
 
    buffer += "lo\r\n0\r\n\r\n";
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Chunked dados parciais: completa ao receber o resto");
 
    check(request.getBody() == "hello",
          "Chunked dados parciais: conteudo do body");
}
 
static void testChunkedSizeLineSplit()
{
    HttpRequest request;
    std::string buffer = chunkedHeaders() + "5";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::PARSING_BODY,
          "Chunked linha de tamanho cortada: continua aguardando");
 
    check(request.getErrorCode() == 0,
          "Chunked linha de tamanho cortada: sem erro");
 
    buffer += "\r\nhello\r\n0\r\n\r\n";
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Chunked linha de tamanho cortada: completa ao receber o resto");
 
    check(request.getBody() == "hello",
          "Chunked linha de tamanho cortada: conteudo do body");
}
 
static void testChunkedByteByByte()
{
    HttpRequest request;
    std::string data =
        chunkedHeaders() + "5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n";
 
    check(feedByteByByte(request, data),
          "Chunked byte a byte: nao completa nem erra antes do ultimo byte");
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Chunked byte a byte: requisicao completa");
 
    check(request.getBody() == "hello world",
          "Chunked byte a byte: conteudo do body");
}
 
static void testChunkedInvalidSize()
{
    HttpRequest request;
    std::string buffer = chunkedHeaders() + "g\r\nhello\r\n0\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Chunked tamanho nao hexa: estado de erro");
 
    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Chunked tamanho nao hexa: STATUS_BAD_REQUEST");
}
 
static void testChunkedNegativeSize()
{
    HttpRequest request;
    std::string buffer = chunkedHeaders() + "-1\r\nhello\r\n0\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Chunked tamanho negativo: estado de erro");
 
    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Chunked tamanho negativo: STATUS_BAD_REQUEST");
}
 
static void testChunkedEmptySize()
{
    HttpRequest request;
    std::string buffer = chunkedHeaders() + "\r\nhello\r\n0\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Chunked linha de tamanho vazia: estado de erro");
 
    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Chunked linha de tamanho vazia: STATUS_BAD_REQUEST");
}
 
static void testChunkedSizeWithSpace()
{
    HttpRequest request;
    std::string buffer = chunkedHeaders() + " 5\r\nhello\r\n0\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Chunked espaco antes do tamanho: estado de erro");
 
    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Chunked espaco antes do tamanho: STATUS_BAD_REQUEST");
}
 
static void testChunkedMissingDataTerminator()
{
    HttpRequest request;
    std::string buffer = chunkedHeaders() + "5\r\nhelloXX0\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Chunked sem CRLF apos os dados: estado de erro");
 
    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Chunked sem CRLF apos os dados: STATUS_BAD_REQUEST");
}
 
static void testChunkedSizeLineTooLongWithoutCrlf()
{
    HttpRequest request;
    std::string buffer = chunkedHeaders() + std::string(100, '1');
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Chunked linha de tamanho enorme sem CRLF: estado de erro");
 
    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Chunked linha de tamanho enorme sem CRLF: STATUS_BAD_REQUEST");
}
 
static void testChunkedSizeLineTooLongWithCrlf()
{
    HttpRequest request;
    std::string buffer =
        chunkedHeaders() + std::string(100, '1') + "\r\nhello\r\n0\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Chunked linha de tamanho enorme com CRLF: estado de erro");
 
    check(request.getErrorCode() == STATUS_BAD_REQUEST,
          "Chunked linha de tamanho enorme com CRLF: STATUS_BAD_REQUEST");
}
 
static void testChunkedSizeOverflow()
{
    // 20 digitos F nao cabem em size_t (64 bits). A RFC exige que o
    // overflow nao quebre o parser; aceito 400 ou 413.
    HttpRequest request;
    std::string buffer =
        chunkedHeaders() + std::string(20, 'F') + "\r\nhello\r\n0\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Chunked tamanho com overflow: estado de erro");
 
    check(request.getErrorCode() == STATUS_BAD_REQUEST ||
          request.getErrorCode() == STATUS_PAYLOAD_TOO_LARGE,
          "Chunked tamanho com overflow: erro apropriado");
}
 
static void testChunkedExceedsLimitSingleChunk()
{
    HttpRequest request(10);
    std::string buffer =
        chunkedHeaders() + "B\r\nhello world\r\n0\r\n\r\n";   // 0xB = 11 bytes
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Chunked 11 bytes com limite 10: estado de erro");
 
    check(request.getErrorCode() == STATUS_PAYLOAD_TOO_LARGE,
          "Chunked 11 bytes com limite 10: STATUS_PAYLOAD_TOO_LARGE");
}
 
static void testChunkedExceedsLimitTotal()
{
    // Cada chunk cabe no limite, mas a soma (12) passa de 10.
    HttpRequest request(10);
    std::string buffer =
        chunkedHeaders() + "6\r\nabcdef\r\n6\r\nghijkl\r\n0\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_ERROR,
          "Chunked soma 12 bytes com limite 10: estado de erro");
 
    check(request.getErrorCode() == STATUS_PAYLOAD_TOO_LARGE,
          "Chunked soma 12 bytes com limite 10: STATUS_PAYLOAD_TOO_LARGE");
}
 
static void testChunkedAtLimit()
{
    HttpRequest request(10);
    std::string buffer =
        chunkedHeaders() + "6\r\nabcdef\r\n4\r\nghij\r\n0\r\n\r\n";
 
    request.feed(buffer);
 
    check(request.getState() == HttpRequest::REQUEST_COMPLETE,
          "Chunked soma 10 bytes com limite 10: aceito");
 
    check(request.getBody() == "abcdefghij",
          "Chunked soma 10 bytes com limite 10: conteudo do body");
}
 
int main()
{
    std::cout << "testValidRequest" << std::endl;
    testValidRequest();
 
    std::cout << "testQueryString" << std::endl;
    testQueryString();
 
    std::cout << "testSupportedMethods" << std::endl;
    testSupportedMethods();
 
    std::cout << "testMalformedRequest" << std::endl;
    testMalformedRequest();
 
    std::cout << "testUnknownMethod" << std::endl;
    testUnknownMethod();
 
    std::cout << "testInvalidUri" << std::endl;
    testInvalidUri();
 
    std::cout << "testPathTraversal" << std::endl;
    testPathTraversal();
 
    std::cout << "testUnsupportedVersion" << std::endl;
    testUnsupportedVersion();
 
    std::cout << "testIncompleteLine" << std::endl;
    testIncompleteLine();
 
    std::cout << "testAdditionalData" << std::endl;
    testAdditionalData();
 
    std::cout << "testEmptyLineBeforeRequest" << std::endl;
    testEmptyLineBeforeRequest();
 
    std::cout << "testLongRequestLine" << std::endl;
    testLongRequestLine();
 
    std::cout << "testValidHeaders" << std::endl;
    testValidHeaders();
 
    std::cout << "testMissingHost" << std::endl;
    testMissingHost();
 
    std::cout << "testEmptyHost" << std::endl;
    testEmptyHost();
 
    std::cout << "testDuplicateHost" << std::endl;
    testDuplicateHost();
 
    std::cout << "testHeaderWithoutColon" << std::endl;
    testHeaderWithoutColon();
 
    std::cout << "testInvalidHeaderName" << std::endl;
    testInvalidHeaderName();
 
    std::cout << "testDuplicateContentLength" << std::endl;
    testDuplicateContentLength();
 
    std::cout << "testDuplicateTransferEncoding" << std::endl;
    testDuplicateTransferEncoding();
 
    std::cout << "testInvalidContentLength" << std::endl;
    testInvalidContentLength();
 
    std::cout << "testNegativeContentLength" << std::endl;
    testNegativeContentLength();
 
    std::cout << "testZeroContentLength" << std::endl;
    testZeroContentLength();
 
    std::cout << "testOversizedContentLength" << std::endl;
    testOversizedContentLength();
 
    std::cout << "testUnsupportedTransferEncoding" << std::endl;
    testUnsupportedTransferEncoding();
 
    std::cout << "testConflictingBodyHeaders" << std::endl;
    testConflictingBodyHeaders();
 
    std::cout << "testDuplicateRegularHeader" << std::endl;
    testDuplicateRegularHeader();
 
    std::cout << "testTooManyHeaders" << std::endl;
    testTooManyHeaders();
 
    std::cout << "testHeadersTooLarge" << std::endl;
    testHeadersTooLarge();
    
    std::cout << "testCaseInsensitiveHeaders" << std::endl;
    testCaseInsensitiveHeaders();
 
    std::cout << "testInvalidHeaderValue" << std::endl;
    testInvalidHeaderValue();
 
    std::cout << "testContentLengthOverflow" << std::endl;
    testContentLengthOverflow();
 
    std::cout << "testContentLengthBodyState" << std::endl;
    testContentLengthBodyState();
 
    std::cout << "testMaximumHeaderCount" << std::endl;
    testMaximumHeaderCount();
 
    std::cout << "testHeaderValueNormalization" << std::endl;
    testHeaderValueNormalization();
 
    std::cout << "testTransferEncodingCaseInsensitive" << std::endl;
    testTransferEncodingCaseInsensitive();
 
    std::cout << "testChunkedNotFinalCoding" << std::endl;
    testChunkedNotFinalCoding();
 
    std::cout << "testDefaultLimitBoundary" << std::endl;
    testDefaultLimitBoundary();
 
    std::cout << "testCustomLimitAcceptsEqual" << std::endl;
    testCustomLimitAcceptsEqual();
 
    std::cout << "testCustomLimitRejectsAbove" << std::endl;
    testCustomLimitRejectsAbove();
 
    std::cout << "testCustomLimitAboveDefault" << std::endl;
    testCustomLimitAboveDefault();
 
    std::cout << "testCustomLimitContentLengthOverflow" << std::endl;
    testCustomLimitContentLengthOverflow();
 
    std::cout << "testBodyContentLengthComplete" << std::endl;
    testBodyContentLengthComplete();
 
    std::cout << "testBodyContentLengthHeadersThenBody" << std::endl;
    testBodyContentLengthHeadersThenBody();
 
    std::cout << "testBodyContentLengthInPieces" << std::endl;
    testBodyContentLengthInPieces();
 
    std::cout << "testBodyContentLengthByteByByte" << std::endl;
    testBodyContentLengthByteByByte();
 
    std::cout << "testBodyContentLengthExtraBytes" << std::endl;
    testBodyContentLengthExtraBytes();
 
    std::cout << "testBodyContentLengthBinary" << std::endl;
    testBodyContentLengthBinary();
 
    std::cout << "testBodyContentLengthMissingBytes" << std::endl;
    testBodyContentLengthMissingBytes();
 
    std::cout << "testNoBodyKeepsExtraBytes" << std::endl;
    testNoBodyKeepsExtraBytes();
 
    std::cout << "testZeroContentLengthKeepsExtraBytes" << std::endl;
    testZeroContentLengthKeepsExtraBytes();
 
    std::cout << "testChunkedSingleChunk" << std::endl;
    testChunkedSingleChunk();
 
    std::cout << "testChunkedMultipleChunks" << std::endl;
    testChunkedMultipleChunks();
 
    std::cout << "testChunkedHexSizes" << std::endl;
    testChunkedHexSizes();
 
    std::cout << "testChunkedLeadingZeros" << std::endl;
    testChunkedLeadingZeros();
 
    std::cout << "testChunkedExtensionIgnored" << std::endl;
    testChunkedExtensionIgnored();
 
    std::cout << "testChunkedTrailersIgnored" << std::endl;
    testChunkedTrailersIgnored();
 
    std::cout << "testChunkedDataWithCrlf" << std::endl;
    testChunkedDataWithCrlf();
 
    std::cout << "testChunkedExtraBytes" << std::endl;
    testChunkedExtraBytes();
 
    std::cout << "testChunkedHeadersThenBody" << std::endl;
    testChunkedHeadersThenBody();
 
    std::cout << "testChunkedDataInPieces" << std::endl;
    testChunkedDataInPieces();
 
    std::cout << "testChunkedSizeLineSplit" << std::endl;
    testChunkedSizeLineSplit();
 
    std::cout << "testChunkedByteByByte" << std::endl;
    testChunkedByteByByte();
 
    std::cout << "testChunkedInvalidSize" << std::endl;
    testChunkedInvalidSize();
 
    std::cout << "testChunkedNegativeSize" << std::endl;
    testChunkedNegativeSize();
 
    std::cout << "testChunkedEmptySize" << std::endl;
    testChunkedEmptySize();
 
    std::cout << "testChunkedSizeWithSpace" << std::endl;
    testChunkedSizeWithSpace();
 
    std::cout << "testChunkedMissingDataTerminator" << std::endl;
    testChunkedMissingDataTerminator();
 
    std::cout << "testChunkedSizeLineTooLongWithoutCrlf" << std::endl;
    testChunkedSizeLineTooLongWithoutCrlf();
 
    std::cout << "testChunkedSizeLineTooLongWithCrlf" << std::endl;
    testChunkedSizeLineTooLongWithCrlf();
 
    std::cout << "testChunkedSizeOverflow" << std::endl;
    testChunkedSizeOverflow();
 
    std::cout << "testChunkedExceedsLimitSingleChunk" << std::endl;
    testChunkedExceedsLimitSingleChunk();
 
    std::cout << "testChunkedExceedsLimitTotal" << std::endl;
    testChunkedExceedsLimitTotal();
 
    std::cout << "testChunkedAtLimit" << std::endl;
    testChunkedAtLimit();
 
    std::cout << "\n" << testsPassed << " passaram, "
              << testsFailed << " falharam" << std::endl;
 
    return testsFailed == 0 ? 0 : 1;
}