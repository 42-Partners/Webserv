#include "tests.hpp"

#include <cstring>
#include <iostream>

int g_passed = 0;
int g_failed = 0;
bool g_verbose = false;

int main(int argc, char** argv) {
	if (argc > 1 && std::strcmp(argv[1], "-v") == 0)
		g_verbose = true;

	runParserTests();
	runStatusCodeTests();
	runErrorPageTests();
	runResolveLocationTests();
	runResolveFilePathTests();

	std::cout << "\n" << g_passed << " passaram, " << g_failed << " falharam" << std::endl;

	return g_failed == 0 ? 0 : 1;
}