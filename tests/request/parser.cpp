
#include "HttpRequest/HttpRequest.hpp"
#include "StatusCode/StatusCode.hpp"

#include <iostream>
#include <string>

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

    return 0;
}