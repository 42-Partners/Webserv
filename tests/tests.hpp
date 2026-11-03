#ifndef TESTS_HPP
# define TESTS_HPP

#include <string>
#include <iostream>

#define RUN(t) do { if (g_verbose) std::cout << "== " #t "\n"; t(); } while (0)
extern int  g_passed;
extern int  g_failed;
extern bool g_verbose;

void check(bool condition, const std::string& name);

// parser
void runParserTests();

// status codes
void runStatusCodeTests();

// error pages
void runErrorPageTests();

// location
void runResolveLocationTests();

// file path
void runResolveFilePathTests();

#endif