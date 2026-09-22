#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "ConfigData.hpp"

class Config;

class Parser {
    public:
        static Config parse_tokens(std::vector<std::string> tokens);
    private:
        static void   parse_server(ServerConfig &server, std::string name, std::vector<std::string> args);
        static void   parse_location(LocationConfig &loc, std::string name, std::vector<std::string> args);
};


#endif