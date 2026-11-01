
#include "HttpResponse/ErrorPage.hpp"
#include "StatusCode/StatusCode.hpp"
#include "../tests.hpp"

#include <sstream>

struct StatusCase {
    int code;
    int number;
    const char *phrase;
};

static const StatusCase cases[] = {
    { STATUS_OK,                    200, "OK" },
    { STATUS_CREATED,               201, "Created" },
    { STATUS_NO_CONTENT,            204, "No Content" },
    { STATUS_MOVED_PERMANENTLY,     301, "Moved Permanently" },
    { STATUS_FOUND,                 302, "Found" },
    { STATUS_BAD_REQUEST,           400, "Bad Request" },
    { STATUS_FORBIDDEN,             403, "Forbidden" },
    { STATUS_NOT_FOUND,             404, "Not Found" },
    { STATUS_METHOD_NOT_ALLOWED,    405, "Method Not Allowed" },
    { STATUS_REQUEST_TIMEOUT,       408, "Request Timeout" },
    { STATUS_LENGTH_REQUIRED,       411, "Length Required" },
    { STATUS_CONTENT_TOO_LARGE,     413, "Content Too Large" },
    { STATUS_URI_TOO_LONG,          414, "URI Too Long" },
    { STATUS_HEADERS_TOO_LARGE,     431, "Request Header Fields Too Large" },
    { STATUS_INTERNAL_SERVER_ERROR, 500, "Internal Server Error" },
    { STATUS_NOT_IMPLEMENTED,       501, "Not Implemented" },
    { STATUS_BAD_GATEWAY,           502, "Bad Gateway" },
    { STATUS_GATEWAY_TIMEOUT,       504, "Gateway Timeout" },
    { STATUS_VERSION_NOT_SUPPORTED, 505, "HTTP Version Not Supported" }
};

static const size_t caseCount = sizeof(cases) / sizeof(cases[0]);

void testEnumValues()
{
    for (size_t i = 0; i < caseCount; ++i) {
        std::ostringstream name;
        name << "Enum: " << cases[i].phrase
             << " vale " << cases[i].number;

        check(cases[i].code == cases[i].number, name.str());
    }
}

void testReasonPhrases()
{
    for (size_t i = 0; i < caseCount; ++i) {
        std::ostringstream name;
        name << "reasonPhrase(" << cases[i].number
             << ") == \"" << cases[i].phrase << "\"";

        check(reasonPhrase(cases[i].code) == cases[i].phrase, name.str());
    }
}

void testNoCodeFallsBackToUnknown()
{
    for (size_t i = 0; i < caseCount; ++i) {
        std::ostringstream name;
        name << "reasonPhrase(" << cases[i].number
             << ") nao e Unknown";

        check(reasonPhrase(cases[i].code) != "Unknown", name.str());
    }
}

void testUnknownCodes()
{
    const int unknown[] = { -1, 0, 99, 299, 418, 999 };
    const size_t count = sizeof(unknown) / sizeof(unknown[0]);

    for (size_t i = 0; i < count; ++i) {
        std::ostringstream name;
        name << "Codigo fora do enum (" << unknown[i] << "): Unknown";

        check(reasonPhrase(unknown[i]) == "Unknown", name.str());
    }
}

void testDefaultErrorPage()
{
    for (size_t i = 0; i < caseCount; ++i) {
        if (cases[i].number < 400)
            continue;

        std::ostringstream title;
        title << cases[i].number << " " << cases[i].phrase;

        std::string page = defaultErrorPage(cases[i].code);

        check(page.find("<title>" + title.str() + "</title>")
              != std::string::npos,
              "Pagina " + title.str() + ": titulo com numero e frase");

        check(page.find("<h1>" + title.str() + "</h1>")
              != std::string::npos,
              "Pagina " + title.str() + ": h1 com numero e frase");
    }
}

void testDefaultErrorPageStructure()
{
    std::string page = defaultErrorPage(STATUS_NOT_FOUND);

    check(page.compare(0, 15, "<!DOCTYPE html>") == 0,
          "Pagina padrao: comeca com o doctype");

    check(page.find("<html>") != std::string::npos &&
          page.find("</html>") != std::string::npos,
          "Pagina padrao: tem as tags html");

    check(page.find("<body>") != std::string::npos &&
          page.find("</body>") != std::string::npos,
          "Pagina padrao: tem as tags body");

    check(page.find("<script") == std::string::npos,
          "Pagina padrao: sem script");

    check(defaultErrorPage(STATUS_NOT_FOUND) == page,
          "Pagina padrao: mesma saida em chamadas repetidas");
}

void testDefaultErrorPageUnknownCode()
{
    std::string page = defaultErrorPage(999);

    check(page.find("<title>999 Unknown</title>") != std::string::npos,
          "Pagina padrao para codigo desconhecido: 999 Unknown");
}
