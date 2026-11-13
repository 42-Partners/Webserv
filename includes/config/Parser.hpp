#ifndef PARSER_HPP
#define PARSER_HPP

#include "config/ServerConfig.hpp"
#include "config/Tokenizer.hpp"
#include <sys/stat.h>
#include <unistd.h>

std::vector<ServerConfig> arg_parser( int ac, char** av );
std::vector<ServerConfig> parse_tokens(std::vector<std::string> tokens);

#endif