#include <iostream>
#include <string>

#include "HttpResponse/HttpResponse.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "RequestProcessor/RequestProcessor.hpp"
#include "config/ServerConfig.hpp"
#include "HttpResponse/ErrorPage.hpp"
#include "../tests.hpp"

// --------------------------------------------------
// Default error page tests
// --------------------------------------------------

static void testDefaultErrorPage404() {
    std::string page = defaultErrorPage(404);

    check(!page.empty(),
          "404: default page must not be empty");

    check(page.find("404") != std::string::npos,
          "404: page must contain status code");
}

static void testDefaultErrorPage403() {
    std::string page = defaultErrorPage(403);

    check(!page.empty(),
          "403: default page must not be empty");

    check(page.find("403") != std::string::npos,
          "403: page must contain status code");
}

static void testDefaultErrorPage500() {
    std::string page = defaultErrorPage(500);

    check(!page.empty(),
          "500: default page must not be empty");

    check(page.find("500") != std::string::npos,
          "500: page must contain status code");
}

// --------------------------------------------------
// Test runner
// --------------------------------------------------

void runErrorPageTests() {
    RUN(testDefaultErrorPage404);
    RUN(testDefaultErrorPage403);
    RUN(testDefaultErrorPage500);
}