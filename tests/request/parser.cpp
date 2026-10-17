
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

    return 0;
}